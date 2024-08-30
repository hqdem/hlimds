//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "addition.h"
#include "negation.h"

namespace eda::gate::synthesizer {

model::SubnetID synthNeg(const model::CellTypeAttr &attr) {
  model::SubnetBuilder builder;

  const auto sizeA = attr.getInWidth(0);
  const auto outSize = attr.getOutWidth(0);

  model::Subnet::LinkList inputsForA = builder.addInputs(sizeA);

  builder.addOutputs(
      convertToTwosComplementCode(builder, inputsForA, outSize, true));

  return builder.make();
}

} // namespace eda::gate::synthesizer
