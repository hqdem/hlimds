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

  BoundGNet extractCone(const GNet *sourceNet, GateID root, Cut &cut,
                        const std::vector<GateID> &order) {
    ConeVisitor coneVisitor(cut, root);
    Walker walker(sourceNet, &coneVisitor);
    walker.walk(cut, root, false);

    BoundGNet boundGNet;
    boundGNet.net = std::shared_ptr<GNet>(coneVisitor.getGNet());
    const auto &cutConeMap = coneVisitor.getResultMatch();
    for (const auto &gate: order) {
      boundGNet.inputBindings.push_back(cutConeMap.find(gate)->second);
    }
    return boundGNet;
  }

  BoundGNet extractCone(const GNet *sourceNet, GateID root,
                        const std::vector<GateID> &order) {
    Cut cut(order.begin(), order.end());

    ConeVisitor coneVisitor(cut, root);
    Walker walker(sourceNet, &coneVisitor);
    walker.walk(cut, root, false);

    BoundGNet boundGNet;
    boundGNet.net = std::shared_ptr<GNet>(coneVisitor.getGNet());
    const auto &cutConeMap = coneVisitor.getResultMatch();
    for (const auto &gate: order) {
      boundGNet.inputBindings.push_back(cutConeMap.find(gate)->second);
    }
    return boundGNet;
  }

  void rmRecursive(GNet *net, GateID start) {

    std::vector<GateID> removed;

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
