//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/synthesizer/synthesizer_div.h"

#include <cassert>

namespace eda::gate::synthesizer {

model::SubnetID synthDivS(const eda::gate::model::CellTypeAttr &attr) {
  // FIXME:
  assert(false && "DivS is unsupported");
  return model::OBJ_NULL_ID;
}

model::SubnetID synthDivU(const eda::gate::model::CellTypeAttr &attr) {
  // FIXME:
  assert(false && "DivU is unsupported");
  return model::OBJ_NULL_ID;
}

model::SubnetID synthRemS(const eda::gate::model::CellTypeAttr &attr) {
  // FIXME:
  assert(false && "RemS is unsupported");
  return model::OBJ_NULL_ID;
}

model::SubnetID synthRemU(const eda::gate::model::CellTypeAttr &attr) {
  // FIXME:
  assert(false && "RemU is unsupported");
  return model::OBJ_NULL_ID;
}

model::SubnetID synthModS(const eda::gate::model::CellTypeAttr &attr) {
  // FIXME:
  assert(false && "ModS is unsupported");
  return model::OBJ_NULL_ID;
}

} // namespace eda::gate::synthesizer
