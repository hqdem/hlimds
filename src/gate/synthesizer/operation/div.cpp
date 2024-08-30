//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "div.h"

namespace eda::gate::synthesizer {

model::SubnetID synthDivS(const eda::gate::model::CellTypeAttr &attr) {
  return model::OBJ_NULL_ID /* unsupported */;
}

model::SubnetID synthDivU(const eda::gate::model::CellTypeAttr &attr) {
  return model::OBJ_NULL_ID /* unsupported */;
}

model::SubnetID synthRemS(const eda::gate::model::CellTypeAttr &attr) {
  return model::OBJ_NULL_ID /* unsupported */;
}

model::SubnetID synthRemU(const eda::gate::model::CellTypeAttr &attr) {
  return model::OBJ_NULL_ID /* unsupported */;
}

model::SubnetID synthModS(const eda::gate::model::CellTypeAttr &attr) {
  return model::OBJ_NULL_ID /* unsupported */;
}

} // namespace eda::gate::synthesizer
