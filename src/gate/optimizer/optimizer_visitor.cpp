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

  OptimizerVisitor::OptimizerVisitor()  {}

  void OptimizerVisitor::set(CutStorage *cutStorage,
                             GNet *net, int cutSize) {
    this->cutStorage = cutStorage;
    this->net = net;
    this->cutSize = cutSize;
  }

  VisitorFlags OptimizerVisitor::onNodeBegin(const GateID &node) {
    if (cutStorage->cuts.find(node) == cutStorage->cuts.end()) {
      // If node is not in cutStorage - means, that it is a new node.
      // So we recount cuts for that node.
      CutsFindVisitor finder(cutSize, cutStorage);
      finder.onNodeBegin(node);
    }
    lastNode = node;
    lastCuts = &(cutStorage->cuts[node]);
    return SUCCESS;
  }

  VisitorFlags OptimizerVisitor::onCut(const Cut &cut) {
    if (checkValidCut(cut)) {
      auto func = getTruthTable(lastNode, cut, net);
      auto list = getSubnets(func);
      for(const auto &option : list) {

        // Creating correspondence map for subNet sources and cut.
        std::unordered_map<GateID, GateID> map;

        auto it = cut.begin();
        for(GateID gateId : option.order) {
          if(it == cut.end()) {
            break;
          }
          map[gateId] = *it;
          ++it;
        }

        if (checkOptimize(cut, option, map)) {
          return considerOptimization(cut, option, map);
        }
      }
    }
    return SUCCESS;
  }

  VisitorFlags OptimizerVisitor::onNodeEnd(const GateID &) {
    // Removing invalid nodes.
    for (const auto &it: toRemove) {
      lastCuts->erase(*it);
    }
    toRemove.clear();
    return SUCCESS;
  }

  bool OptimizerVisitor::checkValidCut(const Cut &cut) {
    for (auto node: cut) {
      if (!net->contains(node)) {
        toRemove.emplace_back(&cut);
        return false;
        // Discard trivial cuts.
      } else if (node == lastNode) {
        return false;
      }
    }
    return true;
  }

} // namespace eda::gate::optimizer