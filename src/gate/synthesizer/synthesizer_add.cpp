//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/synthesizer/synthesizer_add.h"

#include <cassert>

namespace eda::gate::synthesizer {

inline void checkSignature(const model::CellTypeAttr &attr) {
  assert(attr.nInPort == 2);
  assert(attr.nOutPort == 1);
}

model::SubnetID synthAdd(const model::CellTypeAttr &attr) {
  checkSignature(attr);
  // FIXME:
  assert(false);
  return model::OBJ_NULL_ID;
}

model::SubnetID synthSub(const model::CellTypeAttr &attr) {
  checkSignature(attr);
  // FIXME:
  assert(false);
  return model::OBJ_NULL_ID;
}

} // namespace eda::gate::synthesizer
