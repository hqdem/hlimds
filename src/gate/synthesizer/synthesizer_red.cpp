//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/synthesizer/synthesizer_red.h"

#include <cassert>

namespace eda::gate::synthesizer {

static inline void checkRedSignature(const model::CellTypeAttr &attr) {
  assert(attr.nInPort == 1);
  assert(attr.nOutPort == 1 && attr.getOutWidth(0) == 1);
}

model::SubnetID synthRAnd(const model::CellTypeAttr &attr) {
  checkRedSignature(attr);
  // TODO:
  return model::OBJ_NULL_ID;
}

model::SubnetID synthROr(const model::CellTypeAttr &attr) {
  checkRedSignature(attr);
  // TODO:
  return model::OBJ_NULL_ID;
}

model::SubnetID synthRXor(const model::CellTypeAttr &attr) {
  checkRedSignature(attr);
  // TODO:
  return model::OBJ_NULL_ID;
}

model::SubnetID synthRNand(const model::CellTypeAttr &attr) {
  checkRedSignature(attr);
  // TODO:
  return model::OBJ_NULL_ID;
}

model::SubnetID synthRNor(const model::CellTypeAttr &attr) {
  checkRedSignature(attr);
  // TODO:
  return model::OBJ_NULL_ID;
}

model::SubnetID synthRXnor(const model::CellTypeAttr &attr) {
  checkRedSignature(attr);
  // TODO:
  return model::OBJ_NULL_ID;
}
 
} // namespace eda::gate::synthesizer
