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
model::SubnetID synthBAnd(const model::CellTypeAttr &attr);
model::SubnetID synthBOr(const model::CellTypeAttr &attr);
model::SubnetID synthBXor(const model::CellTypeAttr &attr);
model::SubnetID synthBNand(const model::CellTypeAttr &attr);
model::SubnetID synthBNor(const model::CellTypeAttr &attr);
model::SubnetID synthBXnor(const model::CellTypeAttr &attr);
 
} // namespace eda::gate::synthesizer
