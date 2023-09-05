//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "associative_balance_visitor.h"

namespace eda::gate::optimizer {

  template<typename Iter>
  struct is_reverse_iterator : std::false_type {};

  template<typename Iter>
  struct is_reverse_iterator<std::reverse_iterator<Iter>>
         : std::integral_constant<bool, !is_reverse_iterator<Iter>::value> {};

  int AssocBalanceVisitor::getBalancesNumber() const {
    return balancesNumber;
  }

  AssocBalanceVisitor::AssocBalanceVisitor(GNet *net, GateDMap &gateDepth) :
                                           net(net),
                                           gateDepth(gateDepth),
                                           balancesNumber(0) {}

  void AssocBalanceVisitor::updateDepth(const GateId &gateId) {
    int depth = 0;
    for (const auto &input : Gate::get(gateId)->inputs()) {
      depth = std::max(depth, gateDepth[input.node()] + 1);
    }
    gateDepth[gateId] = depth;
  }

  bool AssocBalanceVisitor::canBalanceCompl(const GateId &uOpGateId,
                                            const GateId &dOpGateId,
                                            const GateId &uOpSwapInput) const {

    const Gate *uGate = Gate::get(uOpGateId);
    const Gate *dGate = Gate::get(dOpGateId);
    const GNet::SignalList &uInputs = uGate->inputs();
    const GNet::SignalList &dInputs = dGate->inputs();

    if (!uGate->isMaj() || !dGate->isMaj()) {
      return false;
    }
    if (uInputs.size() != 3 || dInputs.size() != 3) {
      return false;
    }

    GateId uOpInput2 = uInputs[1].node();
    GateId dOpInput2 = dInputs[1].node();
    GateId dOpSwapInput = uInputs[0].node() == dOpGateId ?
                          dInputs[0].node() : dInputs[2].node();

    if (net->getOutEdges(dOpGateId).size() > 1) {
      return false;
    }
    if (uOpInput2 != dOpInput2) {
      return false;
    }
    if (gateDepth[uOpSwapInput] >= gateDepth[dOpSwapInput]) {
      return false;
    }

    return true;
  }

  bool AssocBalanceVisitor::canBalanceAssoc(const GateId &uOpGateId,
                                            const GateId &dOpGateId) const {

    const Gate *uOpGate = Gate::get(uOpGateId);
    const Gate *dOpGate = Gate::get(dOpGateId);
    if (!uOpGate->func().isAssociative()) {
      return false;
    }
    if (!(uOpGate->func() == dOpGate->func() ||
        (uOpGate->isXor() &&
         dOpGate->isXnor()) ||
        (uOpGate->isXnor() &&
         dOpGate->isXor()))) {
      return false;
    }
    if (net->getOutEdges(dOpGateId).size() > 1) {
      return false;
    }
    return true;
  }

  bool AssocBalanceVisitor::canBalance(const GateId &uOpGateId,
                                       const GateId &dOpGateId,
                                       const GateId &uOpSwapInput) const {

    return canBalanceCompl(uOpGateId, dOpGateId, uOpSwapInput) ||
           canBalanceAssoc(uOpGateId, dOpGateId);
  }

  void AssocBalanceVisitor::balanceComplAssoc(const GateId &gateId) {
    const Gate *gate = Gate::get(gateId);
    const GNet::SignalList &gateInputs = gate->inputs();
    GateId firstInputGate = gateInputs[0].node();
    GateId thirdInputGate = gateInputs[2].node();
    GateId dOperGateId;

    if (canBalance(gateId, firstInputGate, thirdInputGate)) {
      dOperGateId = firstInputGate;
    } else if (canBalance(gateId, thirdInputGate, firstInputGate)) {
      dOperGateId = thirdInputGate;
    } else {
      return;
    }

    const Gate *dOperGate = Gate::get(dOperGateId);
    const GNet::SignalList &dInputs = dOperGate->inputs();
    GNet::SignalList newUGateInputs;
    GNet::SignalList newDGateInputs;

    if (dOperGateId == firstInputGate) {
      newUGateInputs = { gateInputs[0], gateInputs[1], dInputs[0] };
      newDGateInputs = { gateInputs[2], dInputs[1], dInputs[2] };
    } else {
      newUGateInputs = { dInputs[2], gateInputs[1], gateInputs[2] };
      newDGateInputs = { dInputs[0], dInputs[1], gateInputs[0] };
    }

    net->setGate(gateId, gate->func(), newUGateInputs);
    net->setGate(dOperGateId, dOperGate->func(), newDGateInputs);
    updateDepth(dOperGateId);
    updateDepth(gateId);
    balancesNumber += balanceOnGate(dOperGateId);
  }

  int AssocBalanceVisitor::balanceOnGate(const GateId &gateId) {
    int depthBeforeBalancings = gateDepth[gateId];
    Gate *gate = Gate::get(gateId);
    int depthBeforeBalancing;

    do {
      depthBeforeBalancing = gateDepth[gateId];
      if (gate->func().isAssociative()) {
        if (gate->func().isCommutative()) {
          balanceCommutAssoc(gateId);
        } else {
          balanceAssoc(gateId);
        }
      } else if (gate->isMaj()) { // Complementary associative function
        balanceComplAssoc(gateId);
      }
    } while(gateDepth[gateId] < depthBeforeBalancing);

    return depthBeforeBalancings - gateDepth[gateId];
  }

  template<typename Iter>
  void AssocBalanceVisitor::moveOp(const GateId &gateId, const Iter operIter,
                                   const Iter inputsBegin, const Iter inputsEnd,
                                   const Iter dInputsBegin,
                                   const Iter dInputsEnd) {

    const Gate *gate = Gate::get(gateId);
    const GateId dOperGateId = operIter->node();
    const Gate *dOperGate = Gate::get(dOperGateId);
    GNet::SignalList newGateInputs;
    GNet::SignalList newDOperGateInputs;

    for (auto inputsIt = inputsBegin; inputsIt != inputsEnd; inputsIt++) {
      if (inputsIt != operIter) {
        newGateInputs.push_back(*inputsIt);
      } else {
        newGateInputs.push_back(*dInputsBegin);
        newGateInputs.push_back(*inputsIt);

        for (auto dInputsIt = dInputsBegin + 1;
             dInputsIt != dInputsEnd;
             ++dInputsIt) {

          newDOperGateInputs.push_back(*dInputsIt);
        }
        newDOperGateInputs.push_back(*(inputsIt + 1));
        if (is_reverse_iterator<Iter>::value) {
          std::reverse(newDOperGateInputs.begin(), newDOperGateInputs.end());
        }
        inputsIt++;
      }
    }
    if (is_reverse_iterator<Iter>::value) {
      std::reverse(newGateInputs.begin(), newGateInputs.end());
    }

    net->setGate(gateId, gate->func(), newGateInputs);
    net->setGate(dOperGateId, dOperGate->func(), newDOperGateInputs);
    updateDepth(dOperGateId);
    updateDepth(gateId);
    balancesNumber += balanceOnGate(dOperGateId);
  }

  template<typename Iter>
  void AssocBalanceVisitor::moveOpToLim(const GateId &gateId, Iter operIter,
                                        const Iter inputsBegin,
                                        const Iter inputsEnd,
                                        const Iter dOpInputsBegin,
                                        const Iter dOpInputsEnd) {

    const GateId &dOperGateId = operIter->node();
    GateId sideInput = (operIter + 1)->node();
    const int &curGateDepth = gateDepth[gateId];

    while(operIter != (inputsEnd - 1) &&
          canBalance(gateId, dOperGateId, sideInput)) {

      if (gateDepth[sideInput] < curGateDepth - 2 ||
          (gateDepth[sideInput] == curGateDepth - 2 &&
          gateDepth[dOperGateId] == curGateDepth - 1)) {

        moveOp(gateId, operIter, inputsBegin, inputsEnd,
               dOpInputsBegin, dOpInputsEnd);
        operIter++;
        sideInput = (operIter + 1)->node();
      } else {
        break;
      }
    }
  }

  void AssocBalanceVisitor::moveAllOpsLToLim(const GateId &gateId) {
    const GNet::SignalList &gateInputs = Gate::get(gateId)->inputs();
    for (long long i = 1; i < (long long)gateInputs.size(); ++i) {
      GateId dOpGateId = gateInputs[i].node();
      GateId leftGateId = gateInputs[i - 1].node();
      if (!canBalance(gateId, dOpGateId, leftGateId)) {
        continue;
      }
      const GNet::SignalList &dOpGateInputs = Gate::get(dOpGateId)->inputs();
      for (long long j = (long long)dOpGateInputs.size() - 2; j >= 0; --j) {
        GateId ddOpGateInput = dOpGateInputs[j].node();
        GateId ddOpGateRightInput = dOpGateInputs[j + 1].node();
        if (!canBalance(dOpGateId, ddOpGateInput, ddOpGateRightInput)) {
          continue;
        }
        const GNet::SignalList & ddOpGateInputs =
          Gate::get(ddOpGateInput)->inputs();
        moveOpToLim(dOpGateId, dOpGateInputs.begin() + j,
                    dOpGateInputs.begin(), dOpGateInputs.end(),
                    ddOpGateInputs.begin(), ddOpGateInputs.end());
        break;
      }
      long long indexFromEnd = (long long)gateInputs.size() - i - 1;
      moveOpToLim(gateId, gateInputs.rbegin() + (indexFromEnd),
                  gateInputs.rbegin(), gateInputs.rend(),
                  dOpGateInputs.rbegin(), dOpGateInputs.rend());
    }
  }

  void AssocBalanceVisitor::moveAllOpsRToLim(const GateId &gateId) {
    const GNet::SignalList &gateInputs = Gate::get(gateId)->inputs();
    for (long long i = (long long)gateInputs.size() - 2; i >= 0; --i) {
      GateId dOpGateId = gateInputs[i].node();
      GateId rightGateId = gateInputs[i + 1].node();
      if (!canBalance(gateId, dOpGateId, rightGateId)) {
        continue;
      }
      const GNet::SignalList &dOpGateInputs = Gate::get(dOpGateId)->inputs();
      for (long long j = 1; j < (long long)dOpGateInputs.size(); ++j) {
        GateId ddOpGateInput = dOpGateInputs[j].node();
        GateId ddOpGateLeftInput = dOpGateInputs[j - 1].node();
        if (!canBalance(dOpGateId, ddOpGateInput, ddOpGateLeftInput)) {
          continue;
        }
        const GNet::SignalList &ddOpGateInputs =
          Gate::get(ddOpGateInput)->inputs();
        long long indexFromEnd = (long long)dOpGateInputs.size() - j - 1;
        moveOpToLim(dOpGateId, dOpGateInputs.rbegin() + (indexFromEnd),
                    dOpGateInputs.rbegin(), dOpGateInputs.rend(),
                    ddOpGateInputs.rbegin(), ddOpGateInputs.rend());
        break;
      }
      moveOpToLim(gateId, gateInputs.begin() + i,
                  gateInputs.begin(), gateInputs.end(),
                  dOpGateInputs.begin(), dOpGateInputs.end());
    }
  }

  void AssocBalanceVisitor::balanceAssoc(const GateId &gateId) {
    moveAllOpsLToLim(gateId);
    moveAllOpsRToLim(gateId);
  }

  void AssocBalanceVisitor::balanceCommutAssoc(const GateId &gateId) {
    const Gate *gate = Gate::get(gateId);
    const GNet::SignalList &gateInputs = Gate::get(gateId)->inputs();
    GNet::SignalList newGateInputs = gateInputs;
    const int curDepth = gateDepth[gateId];

    /* Adding all input gates in increasing depth order. */
    std::multimap<int, GNet::Signal> depthSignal;
    for (const auto &input : gateInputs) {
      GateId inGateId = input.node();
      depthSignal.insert({ gateDepth[inGateId], input });
    }

    for (const auto &input : gateInputs) {
      GateId inGateId = input.node();
      const Gate *inputGate = Gate::get(inGateId);
      const GNet::SignalList &inGateInputs = inputGate->inputs();
      GNet::SignalList newInGateInputs = inGateInputs;

      if (!canBalanceAssoc(gateId, inGateId) ||
          !inputGate->func().isCommutative() ||
          gateDepth[inGateId] != curDepth - 1) {
        continue;
      }
      for (const auto &inputSignalToSwap : inGateInputs) {
        GateId inputSignalId = inputSignalToSwap.node();
        if (depthSignal.empty() || depthSignal.begin()->first >= curDepth - 2) {
          break;
        }
        if (gateDepth[inputSignalId] != curDepth - 2) {
          continue;
        }

        GNet::Signal signalToSwap = depthSignal.begin()->second;
        auto swapIter = std::find(
          newGateInputs.begin(), newGateInputs.end(), signalToSwap
        );
        auto swapIter2 = std::find(
          newInGateInputs.begin(), newInGateInputs.end(), inputSignalToSwap
        );
        newGateInputs.erase(swapIter);
        newInGateInputs.erase(swapIter2);
        depthSignal.erase(depthSignal.begin());
        newGateInputs.push_back(inputSignalToSwap);
        newInGateInputs.push_back(signalToSwap);

        net->setGate(inGateId, inputGate->func(), newInGateInputs);
        updateDepth(inGateId);
        balancesNumber += balanceOnGate(inGateId);
      }
    }
    net->setGate(gateId, gate->func(), newGateInputs);
    updateDepth(gateId);
  }

  VisitorFlags AssocBalanceVisitor::onNodeBegin(const GateId &gateId) {
    updateDepth(gateId);

    return VisitorFlags::CONTINUE;
  }

  VisitorFlags AssocBalanceVisitor::onNodeEnd(const GateId &gateId) {
    balancesNumber += balanceOnGate(gateId);

    return VisitorFlags::CONTINUE;
  }
} // namespace eda::gate::optimizer
