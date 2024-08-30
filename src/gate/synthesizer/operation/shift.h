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

model::SubnetID synthShl(const model::CellTypeAttr &attr);

model::SubnetID synthShrS(const model::CellTypeAttr &attr);

model::SubnetID synthShrU(const model::CellTypeAttr &attr);

model::Subnet::LinkList synthDefaultShiftR(model::SubnetBuilder &builder,
                                           model::Subnet::LinkList &inputs,
                                           const uint16_t sizeMux,
                                           const uint16_t sizeOutput,
                                           bool useSign = false);

model::Subnet::LinkList
synthMuxForShift(model::SubnetBuilder &builder,
                 const model::Subnet::LinkList muxInputs,
                 const uint64_t targetOutputsSize);

} // namespace eda::gate::synthesizer
