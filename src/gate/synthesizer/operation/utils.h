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

void extend(
    model::SubnetBuilder &builder,
    model::Subnet::LinkList &word,
    const uint32_t width,
    const bool signExtend);

void extendOutput(
    model::SubnetBuilder &builder,
    const uint32_t width,
    const bool signExtend);

model::Subnet::LinkList twosComplement(
    model::SubnetBuilder &builder,
    const model::Subnet::LinkList &word,
    const uint32_t width,
    const bool signExtend);

} // namespace eda::gate::synthesizer
