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

model::SubnetID synthMulS(const model::CellTypeAttr &attr);
model::SubnetID synthMulU(const model::CellTypeAttr &attr);

model::Subnet::LinkList
synthKaratsubaMultiplyer(model::SubnetBuilder &builder,
                         model::Subnet::LinkList &inputsForA,
                         model::Subnet::LinkList &inputsForB,
                         const uint16_t outSize, bool useSign = false);

model::Subnet::LinkList
synthSimpleMultipyer(model::SubnetBuilder &builder,
                     model::Subnet::LinkList &inputsForA,
                     model::Subnet::LinkList &inputsForB,
                     const uint16_t outSize, bool useSign = false);

} // namespace eda::gate::synthesizer
