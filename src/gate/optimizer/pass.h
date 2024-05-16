//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"
#include "gate/optimizer/resubstitutor.h"
#include "gate/optimizer/rewriter.h"
#include "gate/optimizer/synthesis/abc_npn4.h"
#include "gate/optimizer/transformer.h"

#include "fmt/format.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <sstream>
#include <string>

namespace eda::gate::optimizer {

using Pass = std::shared_ptr<SubnetInPlaceTransformer>;
using Chain = SubnetInPlaceTransformerChain;
using Effect = model::SubnetBuilder::Effect;

//===----------------------------------------------------------------------===//
// Balance (b)
//===----------------------------------------------------------------------===//

inline Pass b() {
  // FIXME:
  return nullptr;
}

//===----------------------------------------------------------------------===//
// Rewrite (rw)
//===----------------------------------------------------------------------===//

inline Pass rw(const std::string &name, uint16_t k, bool z) {
  static Resynthesizer resynthesizer(AbcNpn4Synthesizer::get());
  return std::make_shared<Rewriter>(
      name, resynthesizer, k, [](const Effect &effect) -> float {
        return static_cast<float>(effect.size);
      }, z);
}

inline Pass rw() {
  return rw("rw", 4, false);
}

inline Pass rwz() {
  return rw("rwz", 4, true);
}

//===----------------------------------------------------------------------===//
// Refactor (rf)
//===----------------------------------------------------------------------===//

inline Pass rf() {
  // FIXME:
  return nullptr;
}

inline Pass rfz() {
  // FIXME:
  return nullptr;
}

//===----------------------------------------------------------------------===//
// Resubstitute (rs)
//===----------------------------------------------------------------------===//

inline Pass rs(const std::string &name, uint16_t k, uint16_t n) {
  return std::make_shared<Resubstitutor>(name, k, n);
}

inline Pass rs(const std::string &name, uint16_t k) {
  return rs(name, k, 16);
}

inline Pass rs(uint16_t k) {
  return rs(fmt::format("rs -K {}", k), k);
}

inline Pass rs() {
  return rs("rs", 8);
}

inline Pass rsz(const std::string &name, uint16_t k, uint16_t n) {
  // FIXME:
  return nullptr;
}

inline Pass rsz(const std::string &name, uint16_t k) {
  return rsz(name, k, 16);
}

inline Pass rsz(uint16_t k) {
  return rsz(fmt::format("rsz -K {}", k), k);
}

inline Pass rsz() {
  return rsz("rsz", 8);
}

//===----------------------------------------------------------------------===//
// Pre-defined scripts
//===----------------------------------------------------------------------===//

inline Pass chain(const std::string &name, const Chain::Chain &chain) {
  return std::make_shared<Chain>(name, chain);
}

/// resyn: b; rw; rwz; b; rwz; b
inline Pass resyn() {
  return chain("resyn",
    {b(), rw(), rwz(), b(), rwz(), b()});
}

/// resyn2: b; rw; rf; b; rw; rwz; b; rfz; rwz; b
inline Pass resyn2() {
  return chain("resyn2",
    {b(), rw(), rf(), b(), rw(), rwz(), b(), rfz(), b()});
}

/// resyn2a: b; rw; b; rw; rwz; b; rwz; b
inline Pass resyn2a() {
  return chain("resyn2a",
    {b(), rw(), b(), rw(), rwz(), b(), rwz(), b()});
}

/// resyn3: b; rs; rs -K 6; b; rsz; rsz -K 6; b; rsz -K 5; b
inline Pass resyn3() {
  return chain("resyn3",
    {b(), rs(), rs(6), b(), rsz(), rsz(6), b(), rsz(5), b()});
}

/// compress: b -l; rw -l; rwz -l; b -l; rwz -l; b -l
inline Pass compress() {
  // TODO: -l
  return chain("compress",
    {b(), rw(), rwz(), b(), rwz(), b()});
}

/// compress2: b -l; rw -l; rf -l; b -l; rw -l; rwz -l; b -l; rfz -l; rwz -l; b -l
inline Pass compress2() {
  // TODO: -l
  return chain("compress2",
    {b(), rw(), rf(), b(), rw(), rwz(), b(), rfz(), rwz(), b()});
}

} // namespace eda::gate::optimizer
