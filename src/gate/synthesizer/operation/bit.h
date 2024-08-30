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

model::SubnetID synthBNot(const model::CellTypeAttr &attr);

model::SubnetID synthBAndS(const model::CellTypeAttr &attr);
model::SubnetID synthBAndU(const model::CellTypeAttr &attr);

model::SubnetID synthBOrS(const model::CellTypeAttr &attr);
model::SubnetID synthBOrU(const model::CellTypeAttr &attr);

model::SubnetID synthBXorS(const model::CellTypeAttr &attr);
model::SubnetID synthBXorU(const model::CellTypeAttr &attr);

model::SubnetID synthBNandS(const model::CellTypeAttr &attr);
model::SubnetID synthBNandU(const model::CellTypeAttr &attr);

model::SubnetID synthBNorS(const model::CellTypeAttr &attr);
model::SubnetID synthBNorU(const model::CellTypeAttr &attr);

model::SubnetID synthBXnorS(const model::CellTypeAttr &attr);
model::SubnetID synthBXnorU(const model::CellTypeAttr &attr);

} // namespace eda::gate::synthesizer
