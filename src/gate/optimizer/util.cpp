//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/util.h"

namespace eda::gate::optimizer {

  std::vector<GNet::GateId> getNext(GateID node, bool forward) {
    std::vector<GNet::GateId> next;
    if (forward) {
      const auto &outputs = Gate::get(node)->links();
      next.reserve(outputs.size());
      for (const auto &out: outputs) {
        next.emplace_back(out.target);
      }
    } else {
      const auto &inputs = Gate::get(node)->inputs();
      next.reserve(inputs.size());
      for (const auto &in: inputs) {
        next.emplace_back(in.node());
      }
    }
    return next;
  }

  void getConeSet(GateID start, std::unordered_set<GateID> &coneNodes,
                  bool forward) {
    std::queue<GateID> bfs;
    bfs.push(start);

    // First trace to define needed nodes.
    while (!bfs.empty()) {
      GateID cur = bfs.front();
      bfs.pop();
      coneNodes.emplace(cur);
      auto next = getNext(cur, forward);
      for (auto node: next) {
        bfs.push(node);
      }
    }
  }

  void getConeSet(GateID start, const Cut &cut,
                  std::unordered_set<GateID> &coneNodes, bool forward) {
    std::queue<GateID> bfs;
    bfs.push(start);

    // First trace to define needed nodes.
    while (!bfs.empty()) {
      GateID cur = bfs.front();
      bfs.pop();
      coneNodes.emplace(cur);
      if (cut.find(cur) != cut.end()) {
        continue;
      }
      auto next = getNext(cur, forward);
      for (auto node: next) {
        bfs.push(node);
      }
    }
  }

  GNet *getCone(const GNet *sourceNet, GateID root, const Cut &cut) {
    return nullptr;
  }

} // namespace eda::gate::optimizer
