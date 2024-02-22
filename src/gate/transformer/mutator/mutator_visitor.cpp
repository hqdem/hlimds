//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gnet.h"
#include "gate/optimizer/optimizer.h"
#include "gate/transformer/mutator/mutator.h"
#include "util/logging.h"

#include <list>

namespace eda::gate::mutator {

  using Gate = eda::gate::model::Gate;
  using GateSymbol = eda::gate::model::GateSymbol;
  using eda::gate::optimizer::getNext;
  using Signal = eda::gate::model::Gate::Signal;
  using SignalList = eda::gate::model::Gate::SignalList;

  MutatorVisitor::MutatorVisitor(const GNet &inputGNet, 
                  int numOfGates,
                  GateIdList &listGates,
                  GateSymbolList &listSymbol) 
                  :
                  replacedFunc(listSymbol),
                  numGates(numOfGates) {
    mVGNet.addNet(inputGNet);
    for (auto *gate : mVGNet.gates()) {
      childGateList.insert(std::pair<GateId, GateIdList>(gate->id(), 
                           getNext(gate->id(), true)));
    }
    replacedGates = filterListGate(listGates);
  }

  VisitorFlags MutatorVisitor::onNodeBegin(const GateId &gateId) {
    auto findGate = std::find(replacedGates.begin(), 
                              replacedGates.end(), 
                              gateId);
    if (findGate == replacedGates.end()) {
      return VisitorFlags::SKIP;
    }
    changeGate(gateId);
    return VisitorFlags::CONTINUE;
  }
  
  VisitorFlags MutatorVisitor::onNodeEnd(const GateId &gateId) {
    return VisitorFlags::CONTINUE;
  }

  void MutatorVisitor::changeGate(const GateId &gateId) {
    std::vector<GateId> parents = getNext(gateId, false);
    SignalList inputs;
    for (size_t i = 0; i < parents.size(); i++) {
      inputs.push_back(Signal::always(parents[i]));
    }
    GateSymbol function = Gate::get(gateId)->func();
    switch (function) {
      case GateSymbol::AND:
      case GateSymbol::XOR:
      case GateSymbol::NAND:
        mVGNet.setGate(gateId, GateSymbol::OR, inputs);
        return;
      case GateSymbol::OR:
      case GateSymbol::NOR:
        mVGNet.setGate(gateId, GateSymbol::AND, inputs);
        return;
      case GateSymbol::XNOR:
        mVGNet.setGate(gateId, GateSymbol::NOR, inputs);
        return;
      default:
        LOG_WARN << "Unexpected symbol: " << function;
        return;
    }
  }

  //function that find path to out from gate (based on BFS)
  bool MutatorVisitor::connectedWithOut(const GateId &startGate) {
    std::vector<GateId> visited;
    std::list<GateId> queue;
    visited.push_back(startGate);
    queue.push_back(startGate);
    while (!queue.empty()) {
      GateId currGate = queue.front();
      queue.pop_front();
      std::vector<GateId> childGates = childGateList[currGate];
      for (GateId gateId : childGates) {
        auto isGateVisited = std::find(visited.begin(), visited.end(), gateId);
        if (isGateVisited == visited.end()) {
          visited.push_back(gateId);
          queue.push_back(gateId);
          if (Gate::get(gateId)->isTarget()) {
            return true;
          }
        }
      }
    }
    return false;
  }

  GateIdList MutatorVisitor::filterListGate(GateIdList &listGate) {
    GateIdList answerList;
    for (GateId gateId : listGate) {
      GateSymbol function = Gate::get(gateId)->func();
      auto findFunc = std::find(replacedFunc.begin(), 
                                replacedFunc.end(), 
                                function);
      bool gateHasOut = connectedWithOut(gateId);
      if ((findFunc != replacedFunc.end()) && 
           gateHasOut && 
           answerList.size() < numGates) {
        answerList.push_back(gateId);
      }
    }
    numChangedGates = answerList.size();
    return answerList;
  }
} //namespace eda::gate::mutator

