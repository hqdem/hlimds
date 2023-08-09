//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "associative_balance_visitor.h"

namespace eda::gate::optimizer {
  int AssociativeBalanceVisitor::getBalancesNumber() const {
    return balancesNumber;
  }

  AssociativeBalanceVisitor::AssociativeBalanceVisitor(GNet *net,
                                                       GateDMap &gateDepth)
                                                       : net(net),
                                                         gateDepth(gateDepth),
                                                         balancesNumber(0) {}

  void AssociativeBalanceVisitor::updateDepth(const GateId &gateId) {
    int depth = 0;
    for (const auto &input : Gate::get(gateId)->inputs()) {
      depth = std::max(depth, gateDepth[input.node()] + 1);
    }
    gateDepth[gateId] = depth;
  }

  bool AssociativeBalanceVisitor::associativeOperations(
    const GateId &gateId1,
    const GateId &gateId2
    ) const {

    Gate *gate1 = Gate::get(gateId1);
    Gate *gate2 = Gate::get(gateId2);
    return
      ((gate1->func() == gate2->func() && gate2->func().isAssociative()) ||
      (gate1->isXor() && gate2->isXnor()) ||
      (gate1->isXnor() && gate2->isXor()));
  }

  bool AssociativeBalanceVisitor::balancableOnGate(const GateId &gateId) {
    // Works only on nets with gates with arity 1 or 2

    const GNet::SignalList &gateInputs = Gate::get(gateId)->inputs();

    // Check if gate has 2 ingoing gates
    if (gateInputs.size() != 2) {
      return false;
    }

    GateId firstInputGate = gateInputs[0].node();
    GateId secondInputGate = gateInputs[1].node();
    // Getting gate to balance
    GateId gateToBalance;
    if (gateDepth[firstInputGate] - gateDepth[secondInputGate] >= 2) {
      gateToBalance = firstInputGate;
    }
    else if (gateDepth[secondInputGate] - gateDepth[firstInputGate] >= 2) {
      gateToBalance = secondInputGate;
    }
    else {
      return false;
    }

    const GNet::SignalList &gateToBalanceInputs = Gate::get(gateToBalance)->inputs();

    // Check if balancing ingoing gate has several parents
    if (net->getOutEdges(gateToBalance).size() > 1 ||
        !associativeOperations(gateToBalance, gateId)) {
      return false;
    }

    // Check if balancing ingoing gate has subnets with the same depth
    if (gateDepth[gateToBalanceInputs[0].node()]
        == gateDepth[gateToBalanceInputs[1].node()]) {
      return false;
    }

    return true;
  }

  void AssociativeBalanceVisitor::balanceOnGates(const GateId &uGateId,
                                                 const GateId &dGateId) {
    // TO DO:
    // ADAPT ALGORITM FOR NETS WITH GATES WITH 3 OR MORE INGOING GATES

    const GNet::SignalList &uGateInputs = Gate::get(uGateId)->inputs();
    const GNet::SignalList &dGateInputs = Gate::get(dGateId)->inputs();
    GNet::SignalList newUGateSignals;
    GNet::SignalList newDGateSignals;

    for (const auto &edge : net->getOutEdges(uGateId)) {
      GateId parentGate = net->leadsTo(edge);
      const GNet::SignalList &parentGateInputs = Gate::get(parentGate)->inputs();
      GNet::SignalList newParentSignals;
      for (const auto &input : parentGateInputs) {
        if (input.node() != uGateId) {
          newParentSignals.push_back(input);
        }
      }
      for (const auto &input : uGateInputs) {
        if (input.node() == dGateId) {
          newParentSignals.push_back(input);
        }
      }
      net->setGate(parentGate, Gate::get(parentGate)->func(), newParentSignals);
    }

    for (const auto &input : uGateInputs) {
      if (input.node() != dGateId) {
        newUGateSignals.push_back(input);
      }
      else {
        newDGateSignals.push_back(GNet::Signal(input.event(), uGateId));
      }
    }

    GateId firstInputGate = dGateInputs[0].node();
    GateId secondInputGate = dGateInputs[1].node();
    GateId smallerDepthInput =
           gateDepth[firstInputGate] < gateDepth[secondInputGate] ?
           firstInputGate : secondInputGate;

    for (const auto &input : dGateInputs) {
      if (gateDepth[input.node()] > gateDepth[smallerDepthInput]) {
        newDGateSignals.push_back(input);
      }
      else {
        newUGateSignals.push_back(input);
      }
    }

    net->setGate(uGateId, Gate::get(uGateId)->func(), newUGateSignals);
    net->setGate(dGateId, Gate::get(dGateId)->func(), newDGateSignals);
  }

  VisitorFlags AssociativeBalanceVisitor::onNodeBegin(const GateId &gateId) {
    updateDepth(gateId);

    return VisitorFlags::CONTINUE;
  }

  VisitorFlags AssociativeBalanceVisitor::onNodeEnd(const GateId &gateId) {
    const GNet::SignalList &gateInputs = Gate::get(gateId)->inputs();

    if (balancableOnGate(gateId)) {
      balancesNumber++;
      GateId dGateId;
      if (gateDepth[gateInputs[0].node()] -
          gateDepth[gateInputs[1].node()] >= 2) {
        dGateId = gateInputs[0].node();
      }
      else {
        dGateId = gateInputs[1].node();
      }

      balanceOnGates(gateId, dGateId);
      // Updating gate depth.
      updateDepth(gateId);
    }

    return VisitorFlags::CONTINUE;
  }
} // namespace eda::gate::optimizer
