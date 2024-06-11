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

model::SubnetID synthEqU(const model::CellTypeAttr &attr);
model::SubnetID synthEqS(const model::CellTypeAttr &attr);

model::SubnetID synthNeqU(const model::CellTypeAttr &attr);
model::SubnetID synthNeqS(const model::CellTypeAttr &attr);

model::SubnetID synthLtU(const model::CellTypeAttr &attr);
model::SubnetID synthLtS(const model::CellTypeAttr &attr);

model::SubnetID synthLteU(const model::CellTypeAttr &attr);
model::SubnetID synthLteS(const model::CellTypeAttr &attr);

model::SubnetID synthGtU(const model::CellTypeAttr &attr);
model::SubnetID synthGtS(const model::CellTypeAttr &attr);

model::SubnetID synthGteU(const model::CellTypeAttr &attr);
model::SubnetID synthGteS(const model::CellTypeAttr &attr);

} // namespace eda::gate::synthesizer