//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "bitwise.h"
#include "utils.h"

namespace eda::gate::synthesizer {

static inline model::SubnetID synthBOp2(const model::CellSymbol symbol,
                                        const model::CellTypeAttr &attr,
                                        const bool signExtend,
                                        const bool positive) {
  model::SubnetBuilder builder;

  const auto wIn0 = attr.getInWidth(0);
  const auto wIn1 = attr.getInWidth(1);
  const auto wOut = attr.getOutWidth(0);

  auto lhs = builder.addInputs(wIn0);
  auto rhs = builder.addInputs(wIn1);

  // If the operand shall be extended, then it shall be sign-extended only
  // if the propagated type is signed [IEEE 1800-2017 11.8.2].
  extend(builder, lhs, wOut, signExtend);
  extend(builder, rhs, wOut, signExtend);

  // Bitwise binary operation.
  for (auto i = 0u; i < wOut; ++i) {
    const auto res = builder.addCell(symbol, lhs[i], rhs[i]);
    builder.addOutput(positive ? res : ~res);
  }

  return builder.make();
}

static inline model::SubnetID synthBNot(const model::CellTypeAttr &attr,
                                        const bool signExtend) {
  model::SubnetBuilder builder;

  const auto wIn = attr.getInWidth(0);
  const auto wOut = attr.getOutWidth(0);

  auto arg = builder.addInputs(wIn);

  // If the operand shall be extended, then it shall be sign-extended only
  // if the propagated type is signed [IEEE 1800-2017 11.8.2].
  extend(builder, arg, wOut, signExtend);

  // Bitwise negation.
  for (auto i = 0u; i < wOut; ++i) {
    builder.addOutput(~arg[i]);
  }

  return builder.make();
}

model::SubnetID synthBNotS(const model::CellTypeAttr &attr) {
  return synthBNot(attr, true);
}

model::SubnetID synthBNotU(const model::CellTypeAttr &attr) {
  return synthBNot(attr, false);
}

model::SubnetID synthBAndS(const model::CellTypeAttr &attr) {
  return synthBOp2(model::AND, attr, true, true);
}

model::SubnetID synthBAndU(const model::CellTypeAttr &attr) {
  return synthBOp2(model::AND, attr, false, true);
}

model::SubnetID synthBOrS(const model::CellTypeAttr &attr) {
  return synthBOp2(model::OR, attr, true, true);
}

model::SubnetID synthBOrU(const model::CellTypeAttr &attr) {
  return synthBOp2(model::OR, attr, false, true);
}

model::SubnetID synthBXorS(const model::CellTypeAttr &attr) {
  return synthBOp2(model::XOR, attr, true, true);
}

model::SubnetID synthBXorU(const model::CellTypeAttr &attr) {
  return synthBOp2(model::XOR, attr, false, true);
}

model::SubnetID synthBNandS(const model::CellTypeAttr &attr) {
  return synthBOp2(model::AND, attr, true, false);
}

model::SubnetID synthBNandU(const model::CellTypeAttr &attr) {
  return synthBOp2(model::AND, attr, false, false);
}

model::SubnetID synthBNorS(const model::CellTypeAttr &attr) {
  return synthBOp2(model::OR, attr, true, false);
}

model::SubnetID synthBNorU(const model::CellTypeAttr &attr) {
  return synthBOp2(model::OR, attr, false, false);
}

model::SubnetID synthBXnorS(const model::CellTypeAttr &attr) {
  return synthBOp2(model::XOR, attr, true, false);
}

model::SubnetID synthBXnorU(const model::CellTypeAttr &attr) {
  return synthBOp2(model::XOR, attr, false, false);
}

} // namespace eda::gate::synthesizer
