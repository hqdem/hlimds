//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/links_add_counter.h"

namespace eda::gate::optimizer {

  LinkAddCounter::LinkAddCounter(const TargetsList &targets,
                                 Visitor::GNet *net,
                                 MatchMap &map,
                                 std::vector<GateID> &toCreate,
                                 std::unordered_set<GateID> &used)
          : targets(targets), net(net), map(map), usedSubNet(used),
            toCreate(toCreate) {}

  VisitorFlags LinkAddCounter::onNodeBegin(const GateID &id) {
    Gate *gate = Gate::get(id);
    // Handling sources.
    if (gate->isSource()) {

      auto found = map.find(id);
      if (found != map.end()) {
        usedSubNet.emplace(id);
        usedNet.emplace(found->second);
      } else {
        // TODO: question N2. implement strategy here.
        //  substitute[id] = Gate::INVALID;

        toCreate.push_back(id);
        return FINISH_FURTHER_NODES;
      }

      // Handling out gate.
    } else if (targets.checkOutGate(gate)) {
      if (toCreate.empty()) {
        toCreate.push_back(gate->id());
      }
      return FINISH_ALL_NODES;
    } else if (!gate->links().empty()) {
      std::vector<Signal> signals;
      // Mapping signals.
      for (const auto &input: gate->inputs()) {
        auto found = map.find(input.node());
        if (found != map.end()) {
          signals.emplace_back(input.event(), found->second);
        } else {
          toCreate.push_back(id);
          return FINISH_FURTHER_NODES;
        }
      }
      const auto *subGate = net->gate(gate->func(), signals);
      if (subGate) {
        usedSubNet.emplace(id);
        usedNet.emplace(subGate->id());
        map[id] = subGate->id();
      } else {
        toCreate.push_back(id);
        return FINISH_FURTHER_NODES;
      }
    }
    return CONTINUE;
  }

  VisitorFlags LinkAddCounter::onNodeEnd(const Visitor::GateID &) {
    return CONTINUE;
  }

  int LinkAddCounter::getUsedNumber() const {
    return static_cast<int>(usedSubNet.size());
  }

  const std::unordered_set<LinkAddCounter::GateID> &
  LinkAddCounter::getUsedNet() {
    return usedNet;
  }

} // namespace eda::gate::optimizer
