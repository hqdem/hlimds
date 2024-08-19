//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/synthesizer/synthesizer_bit.h"

namespace eda::gate::synthesizer {

model::SubnetID synthBNot(const model::CellTypeAttr &attr) {
  model::SubnetBuilder builder;

  const auto arg = builder.addInputs(attr.getInWidth(0));

  for (size_t i = 0; i < arg.size(); ++i) {
    builder.addOutput(~arg[i]);
  }

  return builder.make();
}

static inline model::SubnetID synthBOp2(const model::CellSymbol symbol,
                                        const model::CellTypeAttr &attr,
                                        const bool positive) {
  model::SubnetBuilder builder;

  const auto lhs = builder.addInputs(attr.getInWidth(0));
  const auto rhs = builder.addInputs(attr.getInWidth(1));
  assert(lhs.size() == rhs.size());

  for (size_t i = 0; i < lhs.size(); ++i) {
    const auto res = builder.addCell(symbol, lhs[i], rhs[i]);
    builder.addOutput(positive ? res : ~res);
  }

  return builder.make();
}

model::SubnetID synthBAnd(const model::CellTypeAttr &attr) {
  return synthBOp2(model::AND, attr, true);
}

model::SubnetID synthBOr(const model::CellTypeAttr &attr) {
  return synthBOp2(model::OR, attr, true);
}

model::SubnetID synthBXor(const model::CellTypeAttr &attr) {
  return synthBOp2(model::XOR, attr, true);
}

model::SubnetID synthBNand(const model::CellTypeAttr &attr) {
  return synthBOp2(model::AND, attr, false);
}

model::SubnetID synthBNor(const model::CellTypeAttr &attr) {
  return synthBOp2(model::OR, attr, false);
}

model::SubnetID synthBXnor(const model::CellTypeAttr &attr) {
  return synthBOp2(model::XOR, attr, false);
}
 
} // namespace eda::gate::synthesizer
