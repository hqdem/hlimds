//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/util.h"

#include <queue>

namespace eda::gate::optimizer {

std::vector<GNet::GateId> getNext(GateId node, bool forward) {
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

bool isCut(const GateId &gate, const Cut &cut, GateId &failed) {
  std::queue<GateId> bfs;
  bfs.push(gate);
  while (!bfs.empty()) {
    Gate *cur = Gate::get(bfs.front());
    if (cut.find(cur->id()) == cut.end()) {
      if (cur->isSource()) {
        failed = cur->id();
        return false;
      }
      for (auto input: cur->inputs()) {
        bfs.push(input.node());
      }
    }
    bfs.pop();
  }
  return true;
}

void getConeSet(GateId start, std::unordered_set<GateId> &cone, bool forward) {
  std::queue<GateId> bfs;
  bfs.push(start);

  // First trace to define needed nodes.
  while (!bfs.empty()) {
    GateId cur = bfs.front();
    bfs.pop();
    cone.emplace(cur);
    auto next = getNext(cur, forward);
    for (auto node: next) {
      bfs.push(node);
    }
  }
}

void getConeSet(GateId start, const Cut &cut,
                std::unordered_set<GateId> &coneNodes, bool forward) {
  std::queue<GateId> bfs;
  bfs.push(start);

  // First trace to define needed nodes.
  while (!bfs.empty()) {
    GateId cur = bfs.front();
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

  BoundGNet extractCone(const GNet *net, GateId root, Cut &cut,
                        const Order &order) {
    ConeVisitor coneVisitor(cut, root);
    Walker walker(net, &coneVisitor);
    walker.walk(cut, root, false);

    BoundGNet boundGNet;
    boundGNet.net = std::shared_ptr<GNet>(coneVisitor.getGNet());
    const auto &cutConeMap = coneVisitor.getResultMatch();
    for (const auto &gate: order) {
      boundGNet.inputBindings.push_back(cutConeMap.find(gate)->second);
    }
    return boundGNet;
  }

  BoundGNet extractCone(const GNet *net, GateId root, const Order &order) {
    Cut cut(order.begin(), order.end());

    ConeVisitor coneVisitor(cut, root);
    Walker walker(net, &coneVisitor);
    walker.walk(cut, root, false);

    BoundGNet boundGNet;
    boundGNet.net = std::shared_ptr<GNet>(coneVisitor.getGNet());
    const auto &cutConeMap = coneVisitor.getResultMatch();
    for (const auto &gate: order) {
      auto found = cutConeMap.find(gate);
      if (found == cutConeMap.end()) {
        boundGNet.inputBindings.push_back(Gate::INVALID);
      } else {
        boundGNet.inputBindings.push_back(found->second);
      }
    }
    return boundGNet;
  }

  void rmRecursive(GNet *net, GateId start) {

    std::vector<GateId> removed;

    auto targets = TargetsList(start);

    LinksRemoveCounter removeCounter(targets, {}, removed);
    Walker walker = Walker(net, &removeCounter);
    walker.walk(start, false);

    // Deleting startRm target nodes.
    for (auto node: targets.getTargets()) {
      const auto &outputs = Gate::get(node)->links();
      for (const auto &out: outputs) {
        auto *next = Gate::get(out.target);
        if (next->isTarget()) {
          net->eraseGate(out.target);
        } else {
          auto inputs = next->inputs();
          auto foundId = std::find_if(inputs.begin(), inputs.end(),
                                      [node](const auto &x) {
                                        return x.node() == node;
                                      });
          inputs.erase(foundId);
          net->setGate(out.target, next->func(), inputs);
        }
      }
      net->eraseGate(node);
    }

    // Erasing gates with zero fanout.
    for (auto gate: removed) {
      net->eraseGate(gate);
    }
  }

} // namespace eda::gate::optimizer
