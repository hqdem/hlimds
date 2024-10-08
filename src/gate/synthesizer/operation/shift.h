//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"

namespace eda::gate::synthesizer {

model::SubnetID synthShlS(const model::CellTypeAttr &attr);
model::SubnetID synthShlU(const model::CellTypeAttr &attr);

model::SubnetID synthShrS(const model::CellTypeAttr &attr);
model::SubnetID synthShrU(const model::CellTypeAttr &attr);

model::SubnetID synthShiftS(const model::CellTypeAttr &attr);
model::SubnetID synthShiftU(const model::CellTypeAttr &attr);

} // namespace eda::gate::synthesizer
