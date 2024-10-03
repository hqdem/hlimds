//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "negation.h"
#include "utils.h"

namespace eda::gate::synthesizer {

static inline model::SubnetID synthNeg(const model::CellTypeAttr &attr,
                                       const bool signExtend) {
  model::SubnetBuilder builder;

  const auto wIn = attr.getInWidth(0);
  const auto wOut = attr.getOutWidth(0);

  model::Subnet::LinkList inputs = builder.addInputs(wIn);
  builder.addOutputs(twosComplement(builder, inputs, wOut, signExtend));

  return builder.make();
}

model::SubnetID synthNegS(const model::CellTypeAttr &attr) {
  return synthNeg(attr, true);
}

model::SubnetID synthNegU(const model::CellTypeAttr &attr) {
  return synthNeg(attr, false);
}

} // namespace eda::gate::synthesizer
