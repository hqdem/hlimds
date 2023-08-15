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

  bool AssociativeBalanceVisitor::associative(const GateId &gateId1,
                                              const GateId &gateId2) const {

    Gate *gate1 = Gate::get(gateId1);
    Gate *gate2 = Gate::get(gateId2);
    return
      ((gate1->func() == gate2->func() && gate2->func().isAssociative()) ||
      (gate1->isXor() && gate2->isXnor()) ||
      (gate1->isXnor() && gate2->isXor()));
  }

  bool AssociativeBalanceVisitor::commutative(const GateId &gateId1,
                                              const GateId &gateId2) const {

    Gate *gate1 = Gate::get(gateId1);
    Gate *gate2 = Gate::get(gateId2);
    return
      ((gate1->func() == gate2->func() && gate2->func().isCommutative()) ||
      (gate1->isXor() && gate2->isXnor()) ||
      (gate1->isXnor() && gate2->isXor()));
  }

  AssociativeBalanceVisitor::GateId AssociativeBalanceVisitor::getDeeperInput(
    const GateId &gateId) const {

    GNet::SignalList gateInputs = Gate::get(gateId)->inputs();
    GateId left = gateInputs[0].node();
    GateId right = gateInputs[1].node();
    return gateDepth[left] > gateDepth[right] ? left : right;
  }

  bool AssociativeBalanceVisitor::canReorderOperations(const GateId &gateId) const {
    const GNet::SignalList &gateInputs = Gate::get(gateId)->inputs();

    if (gateInputs.size() != 2) {
      return false;
    }

    GateId firstInputGate = gateInputs[0].node();
    GateId secondInputGate = gateInputs[1].node();

    if (gateDepth[firstInputGate] == gateDepth[secondInputGate]) {
      return false;
    }

    GateId deeperInput = getDeeperInput(gateId);
    const GNet::SignalList &deeperInputInputs = Gate::get(deeperInput)->inputs();
    if (deeperInputInputs.size() != 2 || !associative(deeperInput, gateId)) {
      return false;
    }

    if (net->getOutEdges(deeperInput).size() > 1) {
      return false;
    }

    return true;
  }

  bool AssociativeBalanceVisitor::balanceOnGate(const GateId &gateId) {
    // Works only on nets with gates with arity 1 or 2

    if (!canReorderOperations(gateId)) {
      return false;
    }

    GateId leftInputGate = Gate::get(gateId)->inputs()[0].node();
    GateId rightInputGate = Gate::get(gateId)->inputs()[1].node();
    if (std::abs(gateDepth[leftInputGate] - gateDepth[rightInputGate]) < 2) {
      return false;
    }

    GateId deeperInput = getDeeperInput(gateId);
    GNet::SignalList deeperInputInputs = Gate::get(deeperInput)->inputs();
    GateId lDeeperInputGate = deeperInputInputs[0].node();
    GateId rDeeperInputGate = deeperInputInputs[1].node();
    if (gateDepth[lDeeperInputGate] == gateDepth[rDeeperInputGate]) {
      return false;
    }

    if (commutative(deeperInput, gateId)) {
      return balanceCommutAssoc(gateId, deeperInput);
    } else {
      return balanceAssoc(gateId);
    }

    return false;
  }

  void AssociativeBalanceVisitor::reorderOperationsOnGate(const GateId &gateId) {
    const GNet::SignalList &inputs = Gate::get(gateId)->inputs();
    const GateId lInputGate = inputs[0].node();
    const GateId rInputGate = inputs[1].node();
    if (gateDepth[lInputGate] > gateDepth[rInputGate]) {
      GNet::SignalList newGateSignals {
        Gate::get(lInputGate)->inputs()[0], inputs[0]
      };
      GNet::SignalList newLGateSignals {
        Gate::get(lInputGate)->inputs()[1], inputs[1]
      };
      net->setGate(gateId, Gate::get(gateId)->func(), newGateSignals);
      net->setGate(lInputGate, Gate::get(lInputGate)->func(), newLGateSignals);
      updateDepth(lInputGate);
    } else {
      GNet::SignalList newGateSignals {
        inputs[1], Gate::get(rInputGate)->inputs()[1]
      };
      GNet::SignalList newRGateSignals {
        inputs[0], Gate::get(rInputGate)->inputs()[0]
      };
      net->setGate(gateId, Gate::get(gateId)->func(), newGateSignals);
      net->setGate(rInputGate, Gate::get(rInputGate)->func(), newRGateSignals);
      updateDepth(rInputGate);
    }
    updateDepth(gateId);
  }

  bool AssociativeBalanceVisitor::applyReorderStrategy(const GateId &uGateId,
                                                       const GateId &dGateId,
                                                       const GateId &dGateInput1,
                                                       const GateId &dGateInput2) {

    if (gateDepth[dGateInput1] > gateDepth[dGateInput2]) {
      reorderOperationsOnGate(uGateId);
      return true;
    } else {
      if (!canReorderOperations(dGateId)) {
        return false;
      }
      reorderOperationsOnGate(dGateId);
      reorderOperationsOnGate(uGateId);
      return true;
    }
  }

  bool AssociativeBalanceVisitor::balanceAssoc(const GateId &gateId) {
    GateId lInputGate = Gate::get(gateId)->inputs()[0].node();
    GateId rInputGate = Gate::get(gateId)->inputs()[1].node();
    if (gateDepth[lInputGate] > gateDepth[rInputGate]) {
      GateId llInputGate = Gate::get(lInputGate)->inputs()[0].node();
      GateId lrInputGate = Gate::get(lInputGate)->inputs()[1].node();
      return applyReorderStrategy(gateId, lInputGate, llInputGate, lrInputGate);
    } else {
      GateId rlInputGate = Gate::get(rInputGate)->inputs()[0].node();
      GateId rrInputGate = Gate::get(rInputGate)->inputs()[1].node();
      return applyReorderStrategy(gateId, rInputGate, rrInputGate, rlInputGate);
    }

    return true;
  }

  bool AssociativeBalanceVisitor::balanceCommutAssoc(const GateId &uGateId,
                                                 const GateId &dGateId) {
    // TO DO:
    // ADAPT ALGORITM FOR NETS WITH GATES WITH 3 OR MORE INGOING GATES
    const GNet::SignalList &uGateInputs = Gate::get(uGateId)->inputs();
    const GNet::SignalList &dGateInputs = Gate::get(dGateId)->inputs();
    GNet::SignalList newUGateInputs;
    GNet::SignalList newDGateInputs;
    std::unordered_set<GateId> uGateParents;
    for (const auto &edge : net->getOutEdges(uGateId)) {
      uGateParents.insert(net->leadsTo(edge));
    }

    for (const auto &parent : uGateParents) {
      const GNet::SignalList &parentInputs = Gate::get(parent)->inputs();
      GNet::SignalList newParentInputs;
      for (const auto &input : parentInputs) {
        if (input.node() != uGateId) {
          newParentInputs.push_back(input);
        } else {
          auto dGateInputId = uGateInputs[0].node() == dGateId ? 0 : 1;
          newParentInputs.push_back(uGateInputs[dGateInputId]);
        }
      }
      net->setGate(parent, Gate::get(parent)->func(), newParentInputs);
    }

    for (const auto &input : uGateInputs) {
      if (input.node() != dGateId) {
        newUGateInputs.push_back(input);
      } else {
        newDGateInputs.push_back(GNet::Signal(input.event(), uGateId));
      }
    }

    GateId firstInputGate = dGateInputs[0].node();
    GateId secondInputGate = dGateInputs[1].node();
    GateId smallerDepthInput =
           gateDepth[firstInputGate] < gateDepth[secondInputGate] ?
           firstInputGate : secondInputGate;

    for (const auto &input : dGateInputs) {
      if (gateDepth[input.node()] > gateDepth[smallerDepthInput]) {
        newDGateInputs.push_back(input);
      } else {
        newUGateInputs.push_back(input);
      }
    }

    net->setGate(uGateId, Gate::get(uGateId)->func(), newUGateInputs);
    net->setGate(dGateId, Gate::get(dGateId)->func(), newDGateInputs);

    updateDepth(uGateId);

    return true;
  }

  VisitorFlags AssociativeBalanceVisitor::onNodeBegin(const GateId &gateId) {
    updateDepth(gateId);

    return VisitorFlags::CONTINUE;
  }

  VisitorFlags AssociativeBalanceVisitor::onNodeEnd(const GateId &gateId) {
    if (balanceOnGate(gateId)) {
      balancesNumber++;
    }

    return VisitorFlags::CONTINUE;
  }
} // namespace eda::gate::optimizer
