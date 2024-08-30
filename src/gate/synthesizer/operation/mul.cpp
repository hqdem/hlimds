//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "mul.h"

namespace eda::gate::synthesizer {

model::SubnetID synthMulS(const model::CellTypeAttr &attr) {
  return model::OBJ_NULL_ID /* unsupported */;
}

model::SubnetID synthMulU(const model::CellTypeAttr &attr) {
  return model::OBJ_NULL_ID /* unsupported */;
}

} // namespace eda::gate::synthesizer
