//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#include "gate/tech_optimizer/cut_based_tech_mapper/tech_map_visitor.h"
#include "gate/optimizer/net_substitute.h"
#include "gate/optimizer/optimizer_visitor.h"

#include <float.h>


namespace eda::gate::tech_optimizer {

  using GNet = eda::gate::model::GNet;
  using Gate = eda::gate::model::Gate;
  using RWDatabase = eda::gate::optimizer::RWDatabase;
  using BoundGNetList = eda::gate::optimizer::RWDatabase::BoundGNetList;
  using BoundGNet = eda::gate::optimizer::RWDatabase::BoundGNet;
  using CutStorage = eda::gate::optimizer::CutStorage;
  using VisitorFlags = eda::gate::optimizer::VisitorFlags;
  using Walker = eda::gate::optimizer::Walker;
  using CutsFindVisitor = eda::gate::optimizer::CutsFindVisitor;
  using ConeVisitor = eda::gate::optimizer::ConeVisitor;
  using TruthTable = eda::gate::optimizer::TruthTable;
  using NetSubstitute = eda::gate::optimizer::NetSubstitute;

  SearchOptReplacement::SearchOptReplacement() {}

  void SearchOptReplacement::set(CutStorage *cutStorage,
      GNet *net, 
      std::unordered_map<GateID, Replacement> *bestReplacement,
      int cutSize, RWDatabase &rwdb, Strategy *strategy) {
    this->cutStorage = cutStorage;
    this->net = net;
    this->cutSize = cutSize;
    this->bestReplacement = bestReplacement;
    this->rwdb = rwdb;
    this->strategy = strategy;
  }

  VisitorFlags SearchOptReplacement::onNodeBegin(const GateID &node) {

    if (Gate::get(node)->isTarget()) {
      return eda::gate::optimizer::VisitorFlags::SKIP;
    }
    saveReplace = false;
    minNodeArrivalTime = DBL_MAX;

    if (cutStorage->cuts.find(node) == cutStorage->cuts.end()) {
      CutsFindVisitor finder(cutSize, cutStorage);
      finder.onNodeBegin(node);
    }
    lastNode = node;
    lastCuts = &(cutStorage->cuts[node]);
    return eda::gate::optimizer::VisitorFlags::CONTINUE;
  }

  VisitorFlags SearchOptReplacement::onCut(const GateID &lastNode, const Cut &cut) {
    if (checkValidCut(cut)) {
      // Finding cone.
      ConeVisitor coneVisitor(cut, lastNode);
      Walker walker(net, &coneVisitor);
      walker.walk(cut, lastNode, false);

      // Make binding.
      RWDatabase::BoundGNet boundGNet;
      boundGNet.net = std::shared_ptr<GNet>(coneVisitor.getGNet());

      const auto & cutConeMap = coneVisitor.getResultMatch();
      const auto &resultCut = coneVisitor.getResultCutOldGates();
      for (const auto &gate: resultCut) {
        boundGNet.inputBindings.push_back(cutConeMap.find(gate)->second);
      }

      auto func = TruthTable::build(boundGNet);

      auto list = getSubnets(func);

      for(auto &superGate : list) {
        // Creating correspondence map for subNet sources and cut.
        std::unordered_map<GateID, GateID> map;

        auto it = superGate.inputBindings.begin();
        for (const auto &oldGate: resultCut) {
          if (it != superGate.inputBindings.end()) {
            map[*it] = oldGate;
          } else {
            break;
          }
          ++it;
        }
        
        if (strategy->checkOpt(superGate, map, minNodeArrivalTime,
            bestReplacement)) {
          saveReplace = true;
          return considerTechMap(superGate, map);
        }
      }
    }
    return eda::gate::optimizer::VisitorFlags::CONTINUE;
  }

  VisitorFlags SearchOptReplacement::onNodeEnd(const GateID &) {
    saveBestReplacement();
    for (const auto &it: toRemove) {
      lastCuts->erase(*it);
    }
    toRemove.clear();
    return eda::gate::optimizer::VisitorFlags::CONTINUE;
  }

  bool SearchOptReplacement::checkOptimize(const BoundGNet &superGate,
      const std::unordered_map<GateID, GateID> &map) {
    double maxGateArrivalTime = maxArrivalTime(superGate, map);
    if ( minNodeArrivalTime > maxGateArrivalTime) {
      minNodeArrivalTime = maxGateArrivalTime;
      return true;
    }
    return false;
  }

  VisitorFlags
  SearchOptReplacement::considerTechMap(BoundGNet &superGate,
      std::unordered_map<GateID, GateID> &map) {
    bestOption = superGate;
    bestOptionMap = map;
    return eda::gate::optimizer::VisitorFlags::CONTINUE;
  }

  BoundGNetList
  SearchOptReplacement::getSubnets(uint64_t func) {
    return rwdb.get(func);
  }

  void SearchOptReplacement::saveBestReplacement() {
    if (saveReplace) {

      Replacement bestReplacment{lastNode, eda::gate::model::CELL_TYPE_ID_AND,
          bestOptionMap, bestOption.name,
          minNodeArrivalTime, bestOption.area};

      bestReplacement->insert(std::pair<GateID, Replacement>
          (lastNode, bestReplacment));
      
    } 
  }

  double SearchOptReplacement::maxArrivalTime(const BoundGNet &superGate,
      const std::unordered_map<GateID, GateID> &map) {

    double maxDelay = 0;

    std::unordered_map<Gate::Id, uint32_t> revGareBindings;

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

   bool SearchOptReplacement::checkValidCut(const Cut &cut) {
    for (auto node: cut) {
      if (!net->contains(node)) {
        toRemove.emplace_back(&cut);
        return false;
        // Discard trivial cuts.
      } else if (node == lastNode) {
        return false;
      }
    }
    return true;
  }

} // namespace eda::gate::tech_optimizer
