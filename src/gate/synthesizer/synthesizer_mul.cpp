//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/synthesizer/synthesizer_mul.h"

namespace eda::gate::synthesizer {

model::SubnetID synthMulS(const model::CellTypeAttr &attr) {
  // TODO: Unsupported.
  return model::OBJ_NULL_ID;
}

model::SubnetID synthMulU(const model::CellTypeAttr &attr) {
  // TODO: Unsupported.
  assert(false && "MulU is unsupported");
  return model::OBJ_NULL_ID;
}

} // namespace eda::gate::synthesizer
