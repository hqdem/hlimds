//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/debugger/sat_checker.h"
#include "gate/simulator/simulator.h"
#include "util/logging.h"

namespace eda::gate::debugger {

using Compiled = simulator::Simulator::Compiled;
using Gate = model::Gate;
using GateBinding = std::unordered_map<Gate::Link, Gate::Link>;
using GateId = model::Gate::Id;
using GateIdMap = std::unordered_map<GateId, GateId>;
using GNet = eda::gate::model::GNet;
using Hints = eda::gate::debugger::SatChecker::Hints;
using Signal = model::Gate::Signal;
using SignalList = model::Gate::SignalList;

/**
 * \brief Constructs a miter for the specified nets.
 * @param net1 First net.
 * @param net2 Second net.
 * @param gmap Gate-to-gate mapping between corresponding PI/PO of two nets.
 * @return The miter.
 */
GNet *miter(const GNet &net1, const GNet &net2, const GateIdMap &gmap);

/**
 * \brief Prepares the miter for simulation.
 * @param miter Miter.
 * @return Simulatable representation of a given miter.
 */
Compiled makeCompiled(const GNet &miter);

} // namespace eda::gate::debugger
