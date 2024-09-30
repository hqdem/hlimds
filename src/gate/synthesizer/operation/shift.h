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

model::SubnetID synthShlU(const model::CellTypeAttr &attr);

model::SubnetID synthShlS(const model::CellTypeAttr &attr);

model::SubnetID synthShrS(const model::CellTypeAttr &attr);

model::SubnetID synthShrU(const model::CellTypeAttr &attr);

model::Subnet::LinkList synthDefaultShiftL(model::SubnetBuilder &builder,
                                           const model::CellTypeAttr &attr,
                                           bool useSign = false);

model::Subnet::LinkList synthDefaultShiftR(model::SubnetBuilder &builder,
                                           const model::CellTypeAttr &attr,
                                           bool useSign = false);

model::Subnet::LinkList
synthMuxForShift(model::SubnetBuilder &builder,
                 model::Subnet::LinkList muxInputs,
                 const uint64_t targetOutputsSize);

} // namespace eda::gate::synthesizer
