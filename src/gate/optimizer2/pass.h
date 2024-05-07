//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"
#include "gate/optimizer2/rewriter.h"
#include "gate/optimizer2/synthesis/abc_npn4.h"
#include "gate/optimizer2/transformer.h"

namespace eda::gate::optimizer2 {

using Pass = SubnetInPlaceTransformer;
using Effect = model::SubnetBuilder::Effect;

inline const Pass &rw() {
  static constexpr auto k = 4;
  static Resynthesizer resynthesizer(AbcNpn4Synthesizer::get());
  static Rewriter rewriter(resynthesizer, k, [](const Effect &effect) -> float {
    return (float)effect.size; });
  return rewriter;
}

inline const Pass &rwz() {
  static constexpr auto k = 4;
  static Resynthesizer resynthesizer(AbcNpn4Synthesizer::get());
  static Rewriter rewriter(resynthesizer, k, [](const Effect &effect) -> float {
    return (float)effect.size; }, true);
  return rewriter;
}

} // namespace eda::gate::optimizer2
