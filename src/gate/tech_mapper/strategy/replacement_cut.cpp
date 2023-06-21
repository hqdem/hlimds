//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#include "gate/tech_mapper/strategy/replacement_cut.h"

namespace eda::gate::optimizer {

  using GNet = eda::gate::model::GNet;
  using Gate = eda::gate::model::Gate;

  ReplacementVisitor::ReplacementVisitor() {}

  void ReplacementVisitor::set(CutStorage *cutStorage,
      GNet *net, 
      std::unordered_map<GateID, Replacement> *bestReplacement,
      int cutSize, double &area, double &delay) {
    this->cutStorage = cutStorage;
    this->net = net;
    this->cutSize = cutSize;
    this->bestReplacement = bestReplacement;
    this->area = area;
    this->delay = delay;
                            }
  

  VisitorFlags ReplacementVisitor::onNodeBegin(const GateID &node) {
    lastNode = node;
    finishTechMap();
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

  VisitorFlags ReplacementVisitor::onCut(const Visitor::Cut &cut) {
    return SUCCESS;
  }

  VisitorFlags ReplacementVisitor::onNodeEnd(const GateID &) {

    //finishTechMap();

    // Removing invalid nodes.
    for (const auto &it: toRemove) {
      lastCuts->erase(*it);
    }
    toRemove.clear();
    return SUCCESS;
  }

  bool ReplacementVisitor::checkValidCut(const Cut &cut) {
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

  void ReplacementVisitor::finishTechMap() {
    //std::cout << "Node which we going to rewrite: " << lastNode << std::endl;

    if (bestReplacement->count(lastNode)) {
      if (net->hasNode(lastNode)) {
        Replacement &replacementInfo = bestReplacement->at(lastNode);
        substitute(lastNode, replacementInfo.bestOptionMap, replacementInfo.subsNet, replacementInfo.net);
        std::cout << "substitute: " <<  lastNode << " node to: " << replacementInfo.name << std::endl;
        
        if (delay < replacementInfo.delay) {delay = replacementInfo.delay;}
        std::cout << "delay: " <<delay << std::endl;

        area = area + replacementInfo.area;
        std::cout << "area: " << area << std::endl;
      } else {
        //std::cout << "net hasnt this node" << std::endl;
      }
    } else {
      //std::cout << "replace hasnt this node" << std::endl;
    }
  }


} // namespace eda::gate::optimizer