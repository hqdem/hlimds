//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/techmapper/cut_based_tech_mapper/strategy/min_delay.h"

namespace eda::gate::techmapper {

bool MinDelay::checkOpt(const eda::gate::optimizer::BoundGNet &superGate,
    const eda::gate::model::GNet::GateIdMap &map, double &minNodeArrivalTime,  
    std::unordered_map<GateID, Replacement> *bestSubstitutions) {
    double maxGateArrivalTime = maxArrivalTime(superGate, map, bestSubstitutions);
    if ( minNodeArrivalTime > maxGateArrivalTime) {
      minNodeArrivalTime = maxGateArrivalTime;
      return true;
    }
    return false;
  }

double MinDelay::maxArrivalTime(const BoundGNet &superGate,
    const std::unordered_map<GateID, GateID> &map,
    std::unordered_map<GateID, Replacement> *bestSubstitutions) {

  double maxDelay = 0;

  std::unordered_map<GateID, uint32_t> revGareBindings;

  int superInputId = 0;
  for (const auto &superGateId : superGate.inputBindings) {
      revGareBindings[superGateId] = superInputId;
      superInputId++;
    }

  for (const auto &[inputId, gateId] : map) {
    double delay = 0;

    if (bestSubstitutions->count(gateId)) {
      delay = bestSubstitutions->at(gateId).delay;
    }
    delay = delay + superGate.inputDelays.at(revGareBindings.at(inputId));
    
    if (delay > maxDelay) {
      maxDelay = delay;
    }  
  }
  return maxDelay;
}

} // namespace eda::gate::techmapper
