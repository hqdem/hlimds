//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/optimizer_visitor.h"
#include "gate/printer/dot.h"

namespace eda::gate::optimizer {

  using GNet = eda::gate::model::GNet;
  using Gate = eda::gate::model::Gate;

  OptimizerVisitor::OptimizerVisitor() {}

  void OptimizerVisitor::set(CutStorage *cutStorage,
                             GNet *net, unsigned int cutSize, unsigned int maxCutsNumber) {
    this->cutStorage = cutStorage;
    this->net = net;
    this->cutSize = cutSize;
    this->maxCutsNumber = maxCutsNumber;
  }

  VisitorFlags OptimizerVisitor::onNodeBegin(const GateId &node) {

    if (Gate::get(node)->isTarget()) {
      return SKIP;
    }

    if (cutStorage->cuts.find(node) == cutStorage->cuts.end()) {
      // If node is not in cutStorage - means, that it is a new node.
      // So we recount cuts for that node.
      CutsFindVisitor finder(cutSize, cutStorage, maxCutsNumber);
      finder.onNodeBegin(node);
    }
    lastCuts = &(cutStorage->cuts[node]);

    return CONTINUE;
  }

  VisitorFlags OptimizerVisitor::onCut(const GateId &lastNode, const Cut &cut) {
    if (checkValidCut(lastNode, cut)) {

      // Finding cone.
      ConeVisitor coneVisitor(cut, lastNode);
      Walker walker(net, &coneVisitor);
      walker.walk(cut, lastNode, false);

      // Make binding.
      BoundGNet boundGNet;
      boundGNet.net = std::shared_ptr<GNet>(coneVisitor.getGNet());

      const auto &cutConeMap = coneVisitor.getResultMatch();
      const auto &resultCut = coneVisitor.getResultCutOldGates();
      for (const auto &gate: resultCut) {
        boundGNet.inputBindings.push_back(cutConeMap.find(gate)->second);
      }

      auto func = TruthTable::build(boundGNet);

      auto list = getSubnets(func);
      for (auto &option: list) {

        // TODO: Process constant cuts
        // Creating correspondence map for substNet sources and cut.
        MatchMap map;

        auto it = option.inputBindings.begin();
        for (const auto &oldGate: resultCut) {
          if (it != option.inputBindings.end()) {
            map[*it] = oldGate;
          } else {
            break;
          }
          ++it;
        }

        if (checkOptimize(lastNode, option, map)) {
          considerOptimization(lastNode, option, map);
          return FINISH_FURTHER_NODES;
        }
      }
    }
    return CONTINUE;
  }

  VisitorFlags OptimizerVisitor::onNodeEnd(const GateId &node) {
    // Removing invalid nodes.
    for (const auto &it: toRemove) {
      lastCuts->erase(*it);
    }
    toRemove.clear();
    return finishOptimization(node);
  }

  bool OptimizerVisitor::checkValidCut(const GateId &lastNode, const Cut &cut) {
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
