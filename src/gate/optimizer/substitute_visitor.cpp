//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/substitute_visitor.h"

namespace eda::gate::optimizer {

  SubstituteVisitor::SubstituteVisitor(const TargetsList &targetsList,
                                       GateID cutFor,
                                       MatchMap &map,
                                       GNet *net) :
          targetsList(targetsList), cutFor(cutFor), map(map), net(net) {}

  VisitorFlags SubstituteVisitor::onNodeBegin(const GateID &gateId) {
    Gate *subGate = Gate::get(gateId);

    // If substitute starts from source node,
    // means that source node in substNet doesn't have a pair in the net.
    if (subGate->isSource()) {
      auto found = map.find(gateId);
      assert(found == map.end() &&
             "Substitute only should work with nodes without a match");
      // TODO: implement strategy here.
      map[gateId] = net->newGate();
    } else {
      std::vector<eda::base::model::Signal<GNet::GateId>> signals;

      for (const auto &input: subGate->inputs()) {
        signals.emplace_back(input.event(), map[input.node()]);
      }

      // Handling output gate.
      if (targetsList.checkOutGate(subGate)) {
        map[subGate->id()] = cutFor;
        net->setGate(cutFor, subGate->func(), signals);
        return FINISH_ALL_NODES;
      } else {
        map[subGate->id()] = net->addGate(subGate->func(), signals);
      }
    }
    return CONTINUE;
  }

  VisitorFlags SubstituteVisitor::onNodeEnd(const Visitor::GateID &) {
    return CONTINUE;
  }

}// namespace eda::gate::optimizer
