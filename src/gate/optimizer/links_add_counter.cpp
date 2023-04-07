//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/links_add_counter.h"

namespace eda::gate::optimizer {

  LinkAddCounter::LinkAddCounter(GNet *net, const std::unordered_map<GateID, GateID> &map)
          : net(net), map(map) {}

  VisitorFlags LinkAddCounter::onNodeBegin(const GateID &id) {
    Gate *gate = Gate::get(id);
    // Handling sources.
    if (gate->isSource()) {

      auto found = map.find(id);
      if (found != map.end()) {
        substitute[id] = found->second;
        used.emplace(found->second);
      } else {
        // TODO: implement strategy here.
        substitute[id] = Gate::INVALID;
        ++added;
      }

      // TODO: delete print.
      std::cout << id << " " << substitute[id] << std::endl;
    } else if(!gate->links().empty()) {
      std::vector<Signal> signals;
      // Mapping signals.
      for (const auto &input: gate->inputs()) {
        auto found = substitute.find(input.node());
        if (found != substitute.end()) {
          signals.emplace_back(input.event(), found->second);
        } else {
          ++added;
          return SUCCESS;
        }
      }
      const auto *subGate = net->gate(gate->func(), signals);
      if (subGate) {
        used.emplace(subGate->id());
        substitute[id] = subGate->id();
        // TODO: delete print.
        std::cout << id << " " << subGate->id() << std::endl;
      } else {
        ++added;
      }
    }
    return SUCCESS;
  }

  VisitorFlags LinkAddCounter::onNodeEnd(const Visitor::GateID &) {
    return SUCCESS;
  }

  VisitorFlags LinkAddCounter::onCut(const Visitor::Cut &) {
    return SUCCESS;
  }

} // namespace eda::gate::optimizer
