//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#include "gate/tech_mapper/tech_map_visitor.h"
#include "gate/optimizer/optimizer_visitor.h"


namespace eda::gate::techMap {

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
    saveReplace = false;
    minNodeArrivalTime = std::numeric_limits<double>::max();

    if (cutStorage->cuts.find(node) == cutStorage->cuts.end()) {
      // If the node is not in cutStorage, it is a new node.
      // So we recount cuts for that node.
      CutsFindVisitor finder(cutSize, cutStorage);
      finder.onNodeBegin(node);
    }
    lastNode = node;
    lastCuts = &(cutStorage->cuts[node]);
    return eda::gate::optimizer::VisitorFlags::SUCCESS;
  }

  VisitorFlags SearchOptReplacement::onCut(const Visitor::Cut &cut) {

    if (checkValidCut(cut)) {
      // Finding cone.
      ConeVisitor coneVisitor(cut);
      Walker walker(net, &coneVisitor, nullptr);
      walker.walk(lastNode, cut, false);

      // Make binding.
      RWDatabase::BoundGNet boundGNet;
      boundGNet.net = std::shared_ptr<GNet>(coneVisitor.getGNet());

      const auto & cutConeMap = coneVisitor.getResultCut();
      for(const auto &[gateSource, gateCone] : cutConeMap) {
        boundGNet.inputBindings.push_back(gateCone);
      }

      auto func = TruthTable::build(boundGNet);

      auto list = getSubnets(func);

      for(auto &superGate : list) {
        // Creating correspondence map for subNet sources and cut.
        std::unordered_map<GateID, GateID> map;

        const auto& sources = superGate.net->getSources();
        auto it = sources.begin();
        
        for(const auto & [k, v] : cutConeMap) {
          if(it != sources.end()) {
            map[*it] = k;
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
    return eda::gate::optimizer::VisitorFlags::SUCCESS;
  }

  VisitorFlags SearchOptReplacement::onNodeEnd(const GateID &) {
    saveBestReplacement();
    for (const auto &it: toRemove) {
      lastCuts->erase(*it);
    }
    toRemove.clear();
    return eda::gate::optimizer::VisitorFlags::SUCCESS;
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
    return eda::gate::optimizer::VisitorFlags::SUCCESS;
  }

  BoundGNetList
  SearchOptReplacement::getSubnets(uint64_t func) {
    return rwdb.get(func);
  }

  void SearchOptReplacement::saveBestReplacement() {
    if (saveReplace) {
      Replacement bestReplacment{lastNode, bestOptionMap, bestOption.net.get(), 
        net, minNodeArrivalTime, bestOption.name, bestOption.area};
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

} // namespace eda::gate::techMap