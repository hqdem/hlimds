//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
 #include "gate/tech_mapper/strategy/min_delay.h"

 namespace eda::gate::techMap {
 
 bool MinDelay::checkOpt(const eda::gate::optimizer::BoundGNet &superGate,
      const eda::gate::model::GNet::GateIdMap &map, double &minNodeArrivalTime,  
      std::unordered_map<GateID, Replacement> *bestReplacement) {
      double maxGateArrivalTime = maxArrivalTime(superGate, map, bestReplacement);
      if ( minNodeArrivalTime > maxGateArrivalTime) {
        minNodeArrivalTime = maxGateArrivalTime;
        return true;
      }
      return false;
    }
  
 double MinDelay::maxArrivalTime(const BoundGNet &superGate,
      const std::unordered_map<GateID, GateID> &map,
      std::unordered_map<GateID, Replacement> *bestReplacement) {

    double maxDelay = 0;

    std::unordered_map<GateID, uint32_t> revGareBindings;

    int superInputId = 0;
    for (const auto &superGateId : superGate.inputBindings) {
        revGareBindings[superGateId] = superInputId;
        superInputId++;
      }

    for (const auto &[inputId, gateId] : map) {
      double delay = 0;

      if (bestReplacement->count(gateId)) {
        delay = bestReplacement->at(gateId).delay;
      }
      delay = delay + superGate.inputDelays.at(revGareBindings.at(inputId));
      
      if (delay > maxDelay) {
        maxDelay = delay;
      }  
    }
    return maxDelay;
  }
 } // namespace eda::gate::techMap
