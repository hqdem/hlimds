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

model::SubnetID synthBMux(const model::CellTypeAttr &attr);

model::SubnetID synthMux2(const model::CellTypeAttr &attr);

model::SubnetID synthMux(const model::CellTypeAttr &attr);

model::SubnetID synthBDemux(const model::CellTypeAttr &attr);

model::SubnetID synthDemux2(const model::CellTypeAttr &attr);

model::SubnetID synthDemux(const model::CellTypeAttr &attr);
 
} // namespace eda::gate::synthesizer
