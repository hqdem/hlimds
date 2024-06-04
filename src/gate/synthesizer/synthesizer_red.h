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

model::SubnetID synthRAnd(const model::CellTypeAttr &attr);

model::SubnetID synthROr(const model::CellTypeAttr &attr);

model::SubnetID synthRXor(const model::CellTypeAttr &attr);

model::SubnetID synthRNand(const model::CellTypeAttr &attr);

model::SubnetID synthRNor(const model::CellTypeAttr &attr);

model::SubnetID synthRXnor(const model::CellTypeAttr &attr);
 
} // namespace eda::gate::synthesizer
