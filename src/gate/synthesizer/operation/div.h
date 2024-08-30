//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"

namespace eda::gate::synthesizer {

model::SubnetID synthDivS(const model::CellTypeAttr &attr);
model::SubnetID synthDivU(const model::CellTypeAttr &attr);

model::SubnetID synthRemS(const model::CellTypeAttr &attr);
model::SubnetID synthRemU(const model::CellTypeAttr &attr);

model::SubnetID synthModS(const model::CellTypeAttr &attr);

} // namespace eda::gate::synthesizer
