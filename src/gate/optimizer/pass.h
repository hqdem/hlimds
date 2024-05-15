//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"
#include "gate/optimizer/resubstitutor.h"
#include "gate/optimizer/rewriter.h"
#include "gate/optimizer/synthesis/abc_npn4.h"
#include "gate/optimizer/transformer.h"

#include <memory>

namespace eda::gate::optimizer {

using Pass = SubnetInPlaceTransformer;
using Chain = SubnetInPlaceTransformerChain;
using Effect = model::SubnetBuilder::Effect;

//===----------------------------------------------------------------------===//
// Basic optimization passes
//===----------------------------------------------------------------------===//

inline std::shared_ptr<Pass> b() {
  // FIXME:
  return nullptr;
}

inline std::shared_ptr<Pass> rw(unsigned k, bool z) {
  static Resynthesizer resynthesizer(AbcNpn4Synthesizer::get());
  return std::make_shared<Rewriter>(
      resynthesizer, k, [](const Effect &effect) -> float {
        return static_cast<float>(effect.size);
      }, z);
}

inline std::shared_ptr<Pass> rw() {
  return rw(4, false);
}

inline std::shared_ptr<Pass> rwz() {
  return rw(4, true);
}

inline std::shared_ptr<Pass> rf() {
  // FIXME:
  return nullptr;
}

inline std::shared_ptr<Pass> rfz() {
  // FIXME:
  return nullptr;
}

inline std::shared_ptr<Pass> rs(unsigned k, unsigned careCutSize) {
  return std::make_shared<Resubstitutor>(k, careCutSize);
}

inline std::shared_ptr<Pass> rs(unsigned k) {
  return rs(k, 16);
}

inline std::shared_ptr<Pass> rs() {
  return rs(8);
}

inline std::shared_ptr<Pass> rsz(unsigned k) {
  // FIXME:
  return nullptr;
}

inline std::shared_ptr<Pass> rsz() {
  return rsz(8);
}

//===----------------------------------------------------------------------===//
// Pre-defined optimization chains
//===----------------------------------------------------------------------===//

inline std::shared_ptr<Pass> chain(const Chain::Chain &chain) {
  return std::make_shared<Chain>(chain);
}

/// resyn: b; rw; rwz; b; rwz; b
inline std::shared_ptr<Pass> resyn() {
  return chain({b(), rw(), rwz(), b(), rwz(), b()});
}

/// resyn2: b; rw; rf; b; rw; rwz; b; rfz; rwz; b
inline std::shared_ptr<Pass> resyn2() {
  return chain({b(), rw(), rf(), b(), rw(), rwz(), b(), rfz(), b()});
}

/// resyn2a: b; rw; b; rw; rwz; b; rwz; b
inline std::shared_ptr<Pass> resyn2a() {
  return chain({b(), rw(), b(), rw(), rwz(), b(), rwz(), b()});
}

/// resyn3: b; rs; rs -K 6; b; rsz; rsz -K 6; b; rsz -K 5; b
inline std::shared_ptr<Pass> resyn3() {
  return chain({b(), rs(), rs(6), b(), rsz(), rsz(6), b(), rsz(5), b()});
}

/// compress: b -l; rw -l; rwz -l; b -l; rwz -l; b -l
inline std::shared_ptr<Pass> compress() {
  // TODO: -l
  return chain({b(), rw(), rwz(), b(), rwz(), b()});
}

/// compress2: b -l; rw -l; rf -l; b -l; rw -l; rwz -l; b -l; rfz -l; rwz -l; b -l
inline std::shared_ptr<Pass> compress2() {
  // TODO: -l
  return chain({b(), rw(), rf(), b(), rw(), rwz(), b(), rfz(), rwz(), b()});
}

} // namespace eda::gate::optimizer
