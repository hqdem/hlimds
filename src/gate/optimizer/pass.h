//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"
#include "gate/optimizer/associative_balancer.h"
#include "gate/optimizer/resubstitutor.h"
#include "gate/optimizer/rewriter.h"
#include "gate/optimizer/synthesis/abc_npn4.h"
#include "gate/optimizer/transformer.h"
#include "gate/premapper/aigmapper.h"

#include "fmt/format.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <sstream>
#include <string>

namespace eda::gate::optimizer {

using SubnetPass = std::shared_ptr<SubnetInPlaceTransformer>;
using SubnetMapper = std::shared_ptr<SubnetTransformer>; // TODO:
using SubnetChain = SubnetInPlaceTransformerChain;
using SubnetEffect = model::SubnetBuilder::Effect;

//===----------------------------------------------------------------------===//
// Premappers
//===----------------------------------------------------------------------===//

inline SubnetMapper aig() {
  return std::make_shared<premapper::AigMapper>("aig");
}

inline SubnetMapper mig() {
  // FIXME:
  return nullptr;
}

//===----------------------------------------------------------------------===//
// Balance (b)
//===----------------------------------------------------------------------===//

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

inline SubnetPass rw() {
  return rw("rw", 4, false);
}

inline SubnetPass rwz() {
  return rw("rwz", 4, true);
}

//===----------------------------------------------------------------------===//
// Refactor (rf)
//===----------------------------------------------------------------------===//

inline SubnetPass rf() {
  // FIXME:
  return nullptr;
}

inline SubnetPass rfz() {
  // FIXME:
  return nullptr;
}

// TODO: rfa - area
// TODO: rfd - delay
// TODO: rfp - power

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
// Pre-defined scripts
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
// Technology mappers (map)
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

} // namespace eda::gate::optimizer
