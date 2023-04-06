//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/optimizer_visitor.h"

namespace eda::gate::optimizer {

  using GNet = eda::gate::model::GNet;
  using Gate = eda::gate::model::Gate;
  using Simulator = eda::gate::simulator::Simulator;

  OptimizerVisitor::OptimizerVisitor(CutStorage *cutStorage,
                                     GNet *net) : cutStorage(
          cutStorage), net(net) {}

  VisitorFlags OptimizerVisitor::onNodeBegin(const GateID &node) {
    if (!net->contains(node)) {
      // If node is not in cutStorage - means, that it is a new node.
      // So we recount cuts for that node.
      CutsFindVisitor finder(4, cutStorage);
      finder.onNodeBegin(node);
    }
    lastNode = node;
    lastCuts = &(cutStorage->cuts[node]);
    return SUCCESS;
  }

  VisitorFlags OptimizerVisitor::onCut(const Visitor::Cut &cut) {
    for (auto node: cut) {
      if (!net->contains(node)) {
        toRemove.emplace_back(&cut);
        return SUCCESS;
      // Discard trivial cuts.
      } else if(node == lastNode) {
        return SUCCESS;
      }
    }

    auto func = getTruthTable(lastNode, cut, net);
    // TODO: implement getSubnet method.
    auto *subNet =  getSubnet(func);
    if(fakeSubstitute(lastNode, cut, subNet, net)) {
      substitute(lastNode, cut, subNet, net);
      delete subNet;
      // TODO: we can return finish all here.
      // TODO: we can make list with nets and their profit.
    }

    return SUCCESS;
  }

  VisitorFlags OptimizerVisitor::onNodeEnd(const GateID &) {
    // Removing invalid nodes.
    for(const auto& it : toRemove) {
      lastCuts->erase(*it);
    }
    toRemove.clear();
    return SUCCESS;
  }

  GNet *OptimizerVisitor::getSubnet(uint64_t func) {
    GNet* subNet = new GNet();

    std::vector<eda::base::model::Signal<GNet::GateId>> signals;
    signals.emplace_back(eda::base::model::Event::ALWAYS, subNet->newGate());

    subNet->addGate(model::GateSymbol::Value::AND, signals);
    return subNet;
  }

} // namespace eda::gate::optimizer