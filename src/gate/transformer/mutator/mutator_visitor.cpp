//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gnet.h"
#include "gate/optimizer/optimizer.h"
#include "gate/transformer/mutator/mutator_visitor.h"
#include "util/logging.h"

#include <list>

namespace eda::gate::mutator {

  using Gate = eda::gate::model::Gate;
  using GateSymbol = eda::gate::model::GateSymbol;
  using eda::gate::optimizer::getNext;
  using Signal = eda::gate::model::Gate::Signal;
  using SignalList = eda::gate::model::Gate::SignalList;

  VisitorFlags MutatorVisitor::onNodeBegin(const GateId &gateId) {
    auto findGate = std::find(replacedGates.begin(), replacedGates.end(), gateId);
    if ((findGate == replacedGates.end()) || (numChangedGates >= numGates)) {
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
    for (auto *gate : mVGNet.gates()) {
      childGateList.push_back(getNext(gate->id(), true));
    }
    GateSymbol function = Gate::get(gateId)->func();
    auto findFunc = std::find(replacedFunc.begin(), replacedFunc.end(), function);
    bool gateHasOut = connectedWithOut(gateId);
    if ((findFunc != replacedFunc.end()) && gateHasOut) {
      numChangedGates += 1;
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
  }

  //function that find path to out from gate (based on BFS)
  bool MutatorVisitor::connectedWithOut(const GateId &startGate) {
    std::vector<GateId> visited;
    std::list<GateId> queue;
    GateId firstGateId = mVGNet.gates()[0]->id();
    visited.push_back(startGate);
    queue.push_back(startGate);
    while (!queue.empty()) {
      GateId currGate = queue.front();
      queue.pop_front();
      auto i = childGateList.begin();
      advance(i, (currGate-firstGateId));
      std::vector<GateId> childGates = *i;
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
} //namespace eda::gate::mutator
