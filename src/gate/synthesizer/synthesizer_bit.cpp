//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/synthesizer/synthesizer_bit.h"

#include <cmath>

namespace eda::gate::synthesizer {

static inline void extend(model::SubnetBuilder &builder,
                          model::Subnet::LinkList &word,
                          const uint32_t width,
                          const bool signExtend) {
  if (word.size() >= width) return;

  const auto link = (signExtend && !word.empty())
      ? word.back()
      : builder.addCell(model::ZERO);

  while (word.size() < width) {
    word.push_back(link);
  }
}

static inline model::SubnetID synthBOp2(const model::CellSymbol symbol,
                                        const model::CellTypeAttr &attr,
                                        const bool signExtend,
                                        const bool positive) {
  model::SubnetBuilder builder;

  const auto wIn0 = attr.getInWidth(0);
  const auto wIn1 = attr.getInWidth(1);
  const auto wOut = attr.getOutWidth(0);

  const auto nIn0 = std::min(wIn0, wOut);
  const auto nIn1 = std::min(wIn1, wOut);
  const auto nOut = std::max(nIn0, nIn1);

  auto lhs = builder.addInputs(wIn0);
  auto rhs = builder.addInputs(wIn1);

  // Sign/zero extension of the inputs (if required).
  extend(builder, lhs, nOut, signExtend);
  extend(builder, rhs, nOut, signExtend);

  // Bitwise binary operation.
  for (auto i = 0u; i < nOut; ++i) {
    const auto res = builder.addCell(symbol, lhs[i], rhs[i]);
    builder.addOutput(positive ? res : ~res);
  }

  // Zero extension of the output (if required).
  for (auto i = nOut; i < wOut; ++i) {
    builder.addOutput(builder.addCell(model::ZERO));
  }

  return builder.make();
}

model::SubnetID synthBNot(const model::CellTypeAttr &attr) {
  model::SubnetBuilder builder;

  const auto wIn = attr.getInWidth(0);
  const auto wOut = attr.getOutWidth(0);
  const auto nOut = std::min(wIn, wOut);

  auto arg = builder.addInputs(wIn);

  // Bitwise negation.
  for (auto i = 0u; i < nOut; ++i) {
    builder.addOutput(~arg[i]);
  }

  // Zero extension of the output (if required).
  for (auto i = nOut; i < wOut; ++i) {
    builder.addOutput(builder.addCell(model::ZERO));
  }

  return builder.make();
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
