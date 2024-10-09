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

model::Subnet::Link addMux2(
    model::SubnetBuilder &builder,
    const model::Subnet::Link &s,
    const model::Subnet::Link &x,
    const model::Subnet::Link &y);

model::Subnet::LinkList addMux2(
    model::SubnetBuilder &builder,
    const model::Subnet::Link &s,
    const model::Subnet::LinkList &x);

model::Subnet::LinkList addMux2(
    model::SubnetBuilder &builder,
    const model::Subnet::Link &s,
    const model::Subnet::LinkList &x,
    const model::Subnet::LinkList &y);

std::pair<model::Subnet::Link, model::Subnet::Link> addDemux2(
    model::SubnetBuilder &builder,
    const model::Subnet::Link &s,
    const model::Subnet::Link &x);

model::Subnet::LinkList addDemux2(
    model::SubnetBuilder &builder,
    const model::Subnet::Link &s,
    const model::Subnet::LinkList &x);
 
} // namespace eda::gate::synthesizer
