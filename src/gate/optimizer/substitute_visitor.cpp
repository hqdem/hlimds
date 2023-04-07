//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/substitute_visitor.h"

namespace eda::gate::optimizer {

  SubstituteVisitor::SubstituteVisitor(GateID cutFor,
                                 const std::unordered_map<GateID, GateID> &map,
                                 GNet *subsNet, GNet *net) :
          cutFor(cutFor), map(map), subsNet(subsNet), net(net) {

  }

  VisitorFlags SubstituteVisitor::onNodeBegin(const GateID &gateId) {
    Gate *subGate = Gate::get(gateId);

    // TODO: delete
    std::cout << "SubstituteVisitor.onNodeBegin " << gateId << std::endl;
    // Handling sources of the net.
    if (subGate->isSource()) {
      // TODO: insert map
      auto found = map.find(gateId);
      if (found != map.end()) {
        nodes[gateId] = found->second;
      } else {
        // TODO: implement strategy here.
        nodes[gateId] = net->newGate();
      }
      // TODO: delete
      std::cout << gateId << " -> " << nodes[gateId] << std::endl;
    } else {
      std::vector<eda::base::model::Signal<GNet::GateId>> signals;

      for (const auto &input: subGate->inputs()) {
        signals.emplace_back(input.event(), nodes[input.node()]);
      }

      if (subGate->links().empty()) {
        nodes[subGate->id()] = cutFor;
        assert(nodes.size() == subsNet->nGates()
        && "There is probably more than one out gates in the substitute net.");

        if (Gate::get(cutFor)->inputs() != signals) {
          // Deleting links.
          LinkCleanVisitor visitor(cutFor, net, signals);
          Walker walker(net, &visitor, nullptr);
          walker.walk(cutFor, false);
        }

        return FINISH_ALL;

      } else {
        nodes[subGate->id()] = net->addGate(subGate->func(), signals);
        // TODO: delete
        std::cout << subGate->id() << " -> " << nodes[subGate->id()]
                  << std::endl;
      }
    }

    return SUCCESS;
  }

  VisitorFlags SubstituteVisitor::onNodeEnd(const Visitor::GateID &) {
    return SUCCESS;
  }

  VisitorFlags SubstituteVisitor::onCut(const Visitor::Cut &) {
    return SUCCESS;
  }
} // namespace eda::gate::optimizer
