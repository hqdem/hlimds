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

    cutStorage->cuts.erase(node);
    return SUCCESS;
  }

  VisitorFlags ReplacementVisitor::onCut(const Visitor::Cut &cut) {
    return SUCCESS;
  }

  VisitorFlags ReplacementVisitor::onNodeEnd(const GateID &) {
    return SUCCESS;
  }

  void ReplacementVisitor::finishTechMap() {
    if (bestReplacement->count(lastNode)) {
      if (net->hasNode(lastNode)) {
        Replacement &replacementInfo = bestReplacement->at(lastNode);
        substitute(lastNode, replacementInfo.bestOptionMap, replacementInfo.subsNet, replacementInfo.net);
                
        if (delay < replacementInfo.delay) {delay = replacementInfo.delay;}
        area = area + replacementInfo.area;
      } 
    } 
  }


} // namespace eda::gate::optimizer