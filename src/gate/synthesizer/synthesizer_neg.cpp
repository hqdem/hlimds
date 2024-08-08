//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/synthesizer/synthesizer_add.h"
#include "gate/synthesizer/synthesizer_neg.h"

#include <cassert>

namespace eda::gate::synthesizer {

// Still need to use function from synthesizer_add, 
// and both functions, this and in synthesizer_add has one
// name and are located in same namspace
static inline void checkNegSignature(const model::CellTypeAttr &attr) {
  assert(attr.nInPort == 1);
  assert(attr.nOutPort == 1);
}

model::SubnetID synthNeg(const model::CellTypeAttr &attr) {
  checkNegSignature(attr);

  model::SubnetBuilder builder;

  const auto sizeA = attr.getInWidth(0);
  const auto outSize = attr.getOutWidth(0);

  model::Subnet::LinkList inputsForA = builder.addInputs(sizeA);

  builder.addOutputs(
      convertToTwosComplementCode(builder, inputsForA, outSize, true));

  return builder.make();
}

} // namespace eda::gate::synthesizer
