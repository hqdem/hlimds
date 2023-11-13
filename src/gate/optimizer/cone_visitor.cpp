//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/cone_visitor.h"

namespace eda::gate::optimizer {

  ConeVisitor::ConeVisitor(const Cut &cut, GateId cutFor) : cut(cut),
                                                            cutFor(cutFor) {
    net = new GNet();
  }

  VisitorFlags ConeVisitor::onNodeBegin(const GateId &node) {

    Gate *cur = Gate::get(node);
    const auto &inputs = cur->inputs();
    std::vector<base::model::Signal<GateId>> signals;

    for (const auto &signal: inputs) {
      auto found = newGates.find(signal.node());
      if (found != newGates.end()) {
        signals.emplace_back(base::model::Event::ALWAYS, found->second);
      }
    }

    if (cut.find(node) != cut.end() && signals.empty()) {
      auto func = GateSymbol::IN;
      if (cur->isValue()) {
        func = cur->func();
      }
      auto newGate = net->addGate(func);
      newGates[node] = newGate;
      resultCutOldGates.emplace(node);
    } else {
      newGates[node] = net->addGate(cur->func(), signals);
    }

    if (node == cutFor) {
      if (Gate::get(node)->func() != GateSymbol::OUT) {
        net->addOut(newGates[node]);
      }
      return FINISH_ALL_NODES;
    }

    return CONTINUE;

  }

  VisitorFlags ConeVisitor::onNodeEnd(const GateId &) {
    return CONTINUE;
  }

  ConeVisitor::GNet *ConeVisitor::getGNet() {
    return net;
  }

  const ConeVisitor::MatchMap &ConeVisitor::getResultMatch() {
    return newGates;
  }

  const Cut &ConeVisitor::getResultCutOldGates() {
    return resultCutOldGates;
  }

} // namespace eda::gate::optimizer
