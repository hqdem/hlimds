//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/premapper/xagmapper.h"


using Gate = eda::gate::model::Gate;
using GNet = eda::gate::model::GNet;
using GateBinding = std::unordered_map<Gate::Link, Gate::Link>;
using GateIdMap = std::unordered_map<Gate::Id, Gate::Id>;
using Link = Gate::Link;
using GateSymbol = eda::gate::model::GateSymbol;

void dump(const GNet &net);

std::shared_ptr<GNet> makeSingleGateNet(GateSymbol gate, unsigned N);
std::shared_ptr<GNet> makeSingleGateNetn(GateSymbol gate, unsigned N);
std::shared_ptr<GNet> premap(std::shared_ptr<GNet> net, GateIdMap &gmap);

void initializeBinds(GNet &net, GateIdMap &gmap, GateBinding &ibind, GateBinding &obind);
bool checkEquivalence(std::shared_ptr<GNet> net, std::shared_ptr<GNet> premapped, GateIdMap &gmap);
