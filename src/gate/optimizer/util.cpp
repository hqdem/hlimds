//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/util.h"

namespace eda::gate::optimizer {

  void substitute(GateID cutFor, const std::unordered_map<GateID, GateID> &map, GNet *subsNet, GNet *net) {
    SubstituteVisitor visitor(cutFor, map, subsNet, net);
    Walker walker(subsNet, &visitor, nullptr);
    walker.walk(true);
  }

  int fakeSubstitute(GateID cutFor, const std::unordered_map<GateID, GateID> &map, GNet *subsNet, GNet *net) {

    LinkAddCounter addCounter(net, map);
    Walker walker(subsNet, &addCounter, nullptr);
    walker.walk(true);

    LinksRemoveCounter removeCounter(cutFor, addCounter.getUsed());
    walker = Walker(net, &removeCounter, nullptr);
    walker.walk(cutFor, false);

    return addCounter.getNAdded() - removeCounter.getNRemoved();
  }

  uint64_t getTruthTable(GateID cutFor, const Cut &cut, GNet *net) {
    return 0;
    // Finding cone.
    ConeVisitor coneVisitor(cut);
    Walker walker(net, &coneVisitor, nullptr);
    walker.walk(cutFor, false);

    GNet *subnet = coneVisitor.getGNet();
    // Make binding.
    RWDatabase::GateBindings order;
    RWDatabase::InputId i = 0;
    for(auto gate : cut) {
      order[i++] = gate;
    }
    // TODO: implement via shared ptr.
    // BoundGNet boundGNet = {net, order};

    // call function.
  }
} // namespace eda::gate::optimizer