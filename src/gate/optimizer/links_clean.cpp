//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/links_clean.h"

namespace eda::gate::optimizer {

  LinkCleanVisitor::LinkCleanVisitor(GateID node, GNet *gNet,
                                     const std::vector<eda::base::model::Signal<GNet::GateId>> &newSignals)
          : node(node), newSignals(newSignals),
            gNet(gNet) {}

  VisitorFlags LinkCleanVisitor::onNodeBegin(const GateID &node) {
    // TODO: delete
    std::cout << "onNodeBegin " << node << std::endl;

    if (this->node == node) {
      gNet->setGate(node, Gate::get(node)->func(), newSignals);
      return SUCCESS;
    } else if (Gate::get(node)->fanout() == 0) {
      std::vector<eda::base::model::Signal<GNet::GateId>> signals;
      gNet->setGate(node, GateSymbol::IN, signals);
      gNet->removeGate(node);
      return SUCCESS;
    } else {
      return FINISH_THIS;
    }
  }

  VisitorFlags LinkCleanVisitor::onNodeEnd(const GateID &) {
    return SUCCESS;
  }

  VisitorFlags LinkCleanVisitor::onCut(const Visitor::Cut &) {
    return SUCCESS;
  }
} // namespace eda::gate::optimizer
