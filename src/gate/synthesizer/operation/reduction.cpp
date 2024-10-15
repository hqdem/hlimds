//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "reduction.h"
#include "utils.h"

namespace eda::gate::synthesizer {

static inline model::SubnetID synthROp(const model::CellSymbol symbol,
                                       const model::CellTypeAttr &attr,
                                       const bool positive) {
  model::SubnetBuilder builder;

  const auto arg = builder.addInputs(attr.getInWidth(0));
  const auto res = builder.addCellTree(symbol, arg, 2);
  builder.addOutput(positive ? res : ~res);

  // Comparison and reduction operator results are unsigned,
  // regardless of the operands [IEEE 1800-2017 11.8.1].
  extendOutput(builder, attr.getOutWidth(0), false /* zero extension */);

  return builder.make();
}

model::SubnetID synthRAnd(const model::CellTypeAttr &attr) {
  return synthROp(model::AND, attr, true);
}

model::SubnetID synthROr(const model::CellTypeAttr &attr) {
  return synthROp(model::OR, attr, true);
}

model::SubnetID synthRXor(const model::CellTypeAttr &attr) {
  return synthROp(model::XOR, attr, true);
}

model::SubnetID synthRNand(const model::CellTypeAttr &attr) {
  return synthROp(model::AND, attr, false);
}

model::SubnetID synthRNor(const model::CellTypeAttr &attr) {
  return synthROp(model::OR, attr, false);
}

model::SubnetID synthRXnor(const model::CellTypeAttr &attr) {
  return synthROp(model::XOR, attr, false);
}
 
} // namespace eda::gate::synthesizer
