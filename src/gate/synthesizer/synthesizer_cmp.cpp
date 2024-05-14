//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/synthesizer/synthesizer_cmp.h"

#include <cassert>

namespace eda::gate::synthesizer {

inline void checkSignature(const model::CellTypeAttr &attr) {
  assert(attr.nInPort == 2);
  assert(attr.nOutPort == 1 && attr.getOutWidth(0) == 1);
}

inline model::Subnet::Link synthEq(model::SubnetBuilder &builder,
                                   model::Subnet::LinkList::const_iterator lhs,
                                   model::Subnet::LinkList::const_iterator rhs,
                                   uint16_t width) {
  model::Subnet::LinkList links(width);
  for (uint16_t i = 0; i < width; ++i) {
    links[i] = ~builder.addCell(model::XOR, *lhs++, *rhs++);
  }

  return builder.addCellTree(model::AND, links, 2);
}

inline model::Subnet::Link synthEq(model::SubnetBuilder &builder,
                                   model::Subnet::LinkList::const_iterator lhs,
                                   model::Subnet::Link rhs,
                                   uint16_t width) {
  model::Subnet::LinkList links(width);
  for (uint16_t i = 0; i < width; ++i) {
    links[i] = ~builder.addCell(model::XOR, *lhs++, rhs);
  }

  return builder.addCellTree(model::AND, links, 2);
}

inline model::Subnet::Link synthEqZ(model::SubnetBuilder &builder,
                                    model::Subnet::LinkList::const_iterator lhs,
                                    uint16_t width) {
  model::Subnet::LinkList links(width);
  for (uint16_t i = 0; i < width; ++i) {
    links[i] = *lhs++;
  }

  return builder.addCellTree(model::OR, links, 2);
}

inline model::Subnet::Link synthEq(model::SubnetBuilder &builder,
                                   uint16_t widthLhs,
                                   uint16_t widthRhs,
                                   bool sign) {
  const auto links = builder.addInputs(widthLhs + widthRhs);
  const auto width = std::min(widthLhs, widthRhs);
  const auto delta = std::max(widthLhs, widthRhs) - width;

  const auto valueLhs = links.begin();
  const auto valueRhs = links.begin() + widthLhs;
  const auto equal = synthEq(builder, valueLhs, valueRhs, width);
  
  if (delta == 0) {
    return equal;
  }

  model::Subnet::LinkList::const_iterator signLhs;
  model::Subnet::Link signRhs;

  if (widthLhs < widthRhs) {
    signLhs = links.begin() + 2 * widthLhs;
    signRhs = links[widthLhs - 1];
  } else {
    signLhs = links.begin() + widthRhs;
    signRhs = links.back();
  }

  model::Subnet::Link extended;

  if (sign) {
    extended = synthEq(builder, signLhs, signRhs, delta);
  } else {
    extended = synthEqZ(builder, signLhs, delta);
  }

  return builder.addCell(model::AND, equal, extended);
}

model::SubnetID synthEqS(const model::CellTypeAttr &attr) {
  checkSignature(attr);

  model::SubnetBuilder builder;
  builder.addOutput(
      synthEq(builder, attr.getInWidth(0), attr.getInWidth(1), true));

  return builder.make();
}

model::SubnetID synthEqU(const model::CellTypeAttr &attr) {
  checkSignature(attr);

  model::SubnetBuilder builder;
  builder.addOutput(
      synthEq(builder, attr.getInWidth(0), attr.getInWidth(1), false));

  return builder.make();
}

model::SubnetID synthNeqS(const model::CellTypeAttr &attr) {
  checkSignature(attr);

  model::SubnetBuilder builder;
  builder.addOutput(
      ~synthEq(builder, attr.getInWidth(0), attr.getInWidth(1), true));

  return builder.make();
}

model::SubnetID synthNeqU(const model::CellTypeAttr &attr) {
  checkSignature(attr);

  model::SubnetBuilder builder;
  builder.addOutput(
      ~synthEq(builder, attr.getInWidth(0), attr.getInWidth(1), false));

  return builder.make();
}

model::SubnetID synthLtS(const model::CellTypeAttr &attr) {
  checkSignature(attr);
  // FIXME:
  assert(false);
  return model::OBJ_NULL_ID;
}

model::SubnetID synthLtU(const model::CellTypeAttr &attr) {
  checkSignature(attr);
  // FIXME:
  assert(false);
  return model::OBJ_NULL_ID;
}

model::SubnetID synthLteS(const model::CellTypeAttr &attr) {
  checkSignature(attr);
  // FIXME:
  assert(false);
  return model::OBJ_NULL_ID;
}

model::SubnetID synthLteU(const model::CellTypeAttr &attr) {
  checkSignature(attr);
  // FIXME:
  assert(false);
  return model::OBJ_NULL_ID;
}

model::SubnetID synthGtS(const model::CellTypeAttr &attr) {
  checkSignature(attr);
  // FIXME:
  assert(false);
  return model::OBJ_NULL_ID;
}

model::SubnetID synthGtU(const model::CellTypeAttr &attr) {
  checkSignature(attr);
  // FIXME:
  assert(false);
  return model::OBJ_NULL_ID;
}

model::SubnetID synthGteS(const model::CellTypeAttr &attr) {
  checkSignature(attr);
  // FIXME:
  assert(false);
  return model::OBJ_NULL_ID;
}

model::SubnetID synthGteU(const model::CellTypeAttr &attr) {
  checkSignature(attr);
  // FIXME:
  assert(false);
  return model::OBJ_NULL_ID;
}

} // namespace eda::gate::synthesizer
