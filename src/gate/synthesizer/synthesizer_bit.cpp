//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/synthesizer/synthesizer_bit.h"

#include <cassert>

namespace eda::gate::synthesizer {

static inline void checkBit1Signature(const model::CellTypeAttr &attr) {
  assert(attr.nInPort == 1);
  assert(attr.nOutPort == 1 && attr.getOutWidth(0) == attr.getInWidth(0));
}

static inline void checkBit2Signature(const model::CellTypeAttr &attr) {
  assert(attr.nInPort == 2 && attr.getInWidth(0) == attr.getInWidth(1));
  assert(attr.nOutPort == 1 && attr.getOutWidth(0) == attr.getInWidth(0));
}

model::SubnetID synthBNot(const model::CellTypeAttr &attr) {
  checkBit1Signature(attr);
  // TODO:
  return model::OBJ_NULL_ID;
}

model::SubnetID synthBAnd(const model::CellTypeAttr &attr) {
  checkBit2Signature(attr);
  // TODO:
  return model::OBJ_NULL_ID;
}

model::SubnetID synthBOr(const model::CellTypeAttr &attr) {
  checkBit2Signature(attr);
  // TODO:
  return model::OBJ_NULL_ID;
}

model::SubnetID synthBXor(const model::CellTypeAttr &attr) {
  checkBit2Signature(attr);
  // TODO:
  return model::OBJ_NULL_ID;
}

model::SubnetID synthBNand(const model::CellTypeAttr &attr) {
  checkBit2Signature(attr);
  // TODO:
  return model::OBJ_NULL_ID;
}

model::SubnetID synthBNor(const model::CellTypeAttr &attr) {
  checkBit2Signature(attr);
  // TODO:
  return model::OBJ_NULL_ID;
}

model::SubnetID synthBXnor(const model::CellTypeAttr &attr) {
  checkBit2Signature(attr);
  // TODO:
  return model::OBJ_NULL_ID;
}
 
} // namespace eda::gate::synthesizer
