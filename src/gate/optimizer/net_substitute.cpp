//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/net_substitute.h"

namespace eda::gate::optimizer {

  NetSubstitute::NetSubstitute(GateId cutFor,
                               Visitor::MatchMap *map,
                               GNet *substNet,
                               GNet *net) : cutFor(cutFor),
                                            substNet(substNet),
                                            net(net), map(map),
                                            targetGates(TargetsList(substNet)) {
    assert(!Gate::get(cutFor)->isTarget());
  }

  NetSubstitute::NetSubstitute() : cutFor(Gate::INVALID), substNet(nullptr),
                                   net(nullptr), map(nullptr) {

  }

  void NetSubstitute::substitute() {
    SubstituteVisitor visitor(targetGates, cutFor, *map, net);
    Walker walker(substNet, &visitor);
    walker.walk(toCreate, used);

    // Erasing gates with zero fanout.
    for (auto gate: removed) {
      net->eraseGate(gate);
    }
    removed.clear();
  }

  int NetSubstitute::fakeSubstitute() {

    LinkAddCounter addCounter(targetGates, net, *map, toCreate, used);
    Walker walker(substNet, &addCounter);
    walker.walk(substNet->getSources(), {});

    LinksRemoveCounter removeCounter(TargetsList(cutFor),
                                     addCounter.getUsedNet(), removed);
    walker = Walker(net, &removeCounter);
    walker.walk(cutFor, false);

    int targetCount = static_cast<int>(targetGates.getTargetsSize());
    int initialNumber = static_cast<int>(substNet->nGates());

    return initialNumber - addCounter.getUsedNumber() - targetCount -
           removeCounter.getNRemoved();
  }

} // namespace eda::gate::optimizer