//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "fmt/format.h"
#include "gate/analyzer/probabilistic_estimate.h"
#include "gate/optimizer/area_resynthesizer.h"
#include "gate/optimizer/associative_balancer.h"
#include "gate/optimizer/design_transformer.h"
#include "gate/optimizer/mffc.h"
#include "gate/optimizer/reconvergence_cut.h"
#include "gate/optimizer/refactorer.h"
#include "gate/optimizer/resubstitutor.h"
#include "gate/optimizer/rewriter.h"
#include "gate/optimizer/synthesis/abc_npn4.h"
#include "gate/optimizer/synthesis/isop.h"
#include "gate/premapper/aigmapper.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <sstream>
#include <string>

namespace eda::gate::optimizer {

using ProbEstimator = eda::gate::analyzer::ProbabilityEstimator;
using SubnetBuilder = model::SubnetBuilder;
using SubnetChain   = SubnetInPlaceTransformerChain;
using SubnetEffect  = model::SubnetBuilder::Effect;

//===----------------------------------------------------------------------===//
// Premappers
//===----------------------------------------------------------------------===//

/// Mapping to the AIG representation.
inline SubnetMapper aig() {
  return std::make_shared<premapper::AigMapper>("aig");
}

/// Mapping to the MIG representation.
inline SubnetMapper mig() {
  // FIXME:
  return nullptr;
}

//===----------------------------------------------------------------------===//
// Balance (b)
//===----------------------------------------------------------------------===//

/// Depth-aware balancing.
inline SubnetPass b() {
  return std::make_shared<AssociativeBalancer>("b");
}

//===----------------------------------------------------------------------===//
// Rewrite (rw)
//===----------------------------------------------------------------------===//

inline SubnetPass rw(const std::string &name, uint16_t k, bool z) {
  static Resynthesizer resynthesizer(AbcNpn4Synthesizer::get());
  return std::make_shared<Rewriter>(
      name, resynthesizer, k, [](const SubnetEffect &effect) -> float {
        return static_cast<float>(effect.size);
      }, z);
}

/// Basic rewriting.
inline SubnetPass rw() {
  return rw("rw", 4, false);
}

/// Rewriting w/ enabled zero-cost replacements.
inline SubnetPass rwz() {
  return rw("rwz", 4, true);
}

//===----------------------------------------------------------------------===//
// Refactor (rf)
//===----------------------------------------------------------------------===//

inline SubnetPass rfarea(const std::string &name,
                         Refactorer::ReplacePredicate *replacePredicate) {
  static AreaResynthesizer resynthesizer;
  static Refactorer::ConeConstructor coneConstructor = 
      [](SubnetBuilder &builder, size_t root, uint16_t cutSize,
        Refactorer::EntryMap &map) {
        auto leaves = getReconvergenceCut(builder, root, cutSize);
        return getMffc(builder, root, leaves, map);
      };
  return std::make_shared<Refactorer>(name,
                                      resynthesizer,
                                      &coneConstructor,
                                      8, 16,
                                      replacePredicate);
}

/// Basic refactoring.
inline SubnetPass rf() {
  static Refactorer::ReplacePredicate replacePredicate =
      [](const SubnetEffect &effect) {
        return effect.size > 0;
      };
  return rfarea("rf", &replacePredicate);
}

/// Refactoring w/ enabled zero-cost replacements.
inline SubnetPass rfz() {
  static Refactorer::ReplacePredicate replacePredicate =
    [](const SubnetEffect &effect) {
      return effect.size >= 0;
    };
  return rfarea("rfz", &replacePredicate);
}

/// Area-aware refactoring.
inline SubnetPass rfa() {
  return rf();
}

/// Delay-aware refactoring.
inline SubnetPass rfd() {
  static synthesis::MMSynthesizer mm;
  static Resynthesizer resynthesizer(mm);
  static Refactorer::ConeConstructor coneConstructor{getReconvergenceCone};
  static Refactorer::ReplacePredicate replacePredicate =
    [](const SubnetEffect &effect) {
      return effect.depth > 0;
    };
  return std::make_shared<Refactorer>("rfd",
                                    resynthesizer,
                                    &coneConstructor,
                                    16, 0,
                                    &replacePredicate);
}

/// Power-aware refactoring.
inline SubnetPass rfp() {
  static synthesis::MMSynthesizer mm;
  static Resynthesizer resynthesizer(mm);
  static Refactorer::ConeConstructor coneConstructor{getReconvergenceCone};
  static const float stage{0.1f};
  static Refactorer::ReplacePredicate replacePredicate =
    [](const SubnetEffect &effect) {
      return effect.weight > stage;
    };
  static Refactorer::WeightCalculator weightCalculator = 
      [](SubnetBuilder & builder, const std::vector<float> &inputWeights) {
        static ProbEstimator estimator;
        const auto weights = estimator.estimateProbs(builder, inputWeights);
        for (auto it{builder.begin()}; it != builder.end(); ++it) {
          builder.setWeight(*it, weights[*it]);
        }
      };
  static Refactorer::CellWeightModifier weightModifier =
      [](float p) {
        return 2 * p * (1.f - p);
      };
  return std::make_shared<Refactorer>("rfp",
                                    resynthesizer,
                                    &coneConstructor,
                                    10, 0,
                                    &replacePredicate,
                                    &weightCalculator,
                                    &weightModifier);
}

//===----------------------------------------------------------------------===//
// Resubstitute (rs)
//===----------------------------------------------------------------------===//

inline SubnetPass rs(const std::string &name, uint16_t k, uint16_t n) {
  return std::make_shared<Resubstitutor>(name, k, n);
}

inline SubnetPass rs(const std::string &name, uint16_t k) {
  return rs(name, k, 16);
}

inline SubnetPass rs(uint16_t k) {
  return rs(fmt::format("rs -K {}", k), k);
}

inline SubnetPass rs() {
  return rs("rs", 8);
}

inline SubnetPass rsz(const std::string &name, uint16_t k, uint16_t n) {
  // FIXME:
  return nullptr;
}

inline SubnetPass rsz(const std::string &name, uint16_t k) {
  return rsz(name, k, 16);
}

inline SubnetPass rsz(uint16_t k) {
  return rsz(fmt::format("rsz -K {}", k), k);
}

inline SubnetPass rsz() {
  return rsz("rsz", 8);
}

//===----------------------------------------------------------------------===//
// Pre-defined Scripts
//===----------------------------------------------------------------------===//

inline SubnetPass chain(const std::string &name,
                        const SubnetChain::Chain &chain) {
  return std::make_shared<SubnetChain>(name, chain);
}

/// resyn: b; rw; rwz; b; rwz; b
inline SubnetPass resyn() {
  return chain("resyn",
    {b(), rw(), rwz(), b(), rwz(), b()});
}

/// resyn2: b; rw; rf; b; rw; rwz; b; rfz; rwz; b
inline SubnetPass resyn2() {
  return chain("resyn2",
    {b(), rw(), rf(), b(), rw(), rwz(), b(), rfz(), b()});
}

/// resyn2a: b; rw; b; rw; rwz; b; rwz; b
inline SubnetPass resyn2a() {
  return chain("resyn2a",
    {b(), rw(), b(), rw(), rwz(), b(), rwz(), b()});
}

/// resyn3: b; rs; rs -K 6; b; rsz; rsz -K 6; b; rsz -K 5; b
inline SubnetPass resyn3() {
  return chain("resyn3",
    {b(), rs(), rs(6), b(), rsz(), rsz(6), b(), rsz(5), b()});
}

/// compress: b -l; rw -l; rwz -l; b -l; rwz -l; b -l
inline SubnetPass compress() {
  // TODO: -l
  return chain("compress",
    {b(), rw(), rwz(), b(), rwz(), b()});
}

/// compress2: b -l; rw -l; rf -l; b -l; rw -l; rwz -l; b -l; rfz -l; rwz -l; b -l
inline SubnetPass compress2() {
  // TODO: -l
  return chain("compress2",
    {b(), rw(), rf(), b(), rw(), rwz(), b(), rfz(), rwz(), b()});
}

//===----------------------------------------------------------------------===//
// Technology Mappers
//===----------------------------------------------------------------------===//

inline SubnetMapper ma() {
  // FIXME:
  return nullptr;
}

inline SubnetMapper md() {
  // FIXME:
  return nullptr;
}

inline SubnetMapper mp() {
  // FIXME:
  return nullptr;
}

//===----------------------------------------------------------------------===//
// Basic Design Passes
//===----------------------------------------------------------------------===//

inline DesignPass foreach(const SubnetPass &pass) {
  return std::make_shared<EachSubnetInPlaceTransformer>(pass);
}

inline DesignPass foreach(const SubnetMapper &mapper) {
  return std::make_shared<EachSubnetTransformer>(mapper);
}

} // namespace eda::gate::optimizer
