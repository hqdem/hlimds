//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/cone_visitor.h"

namespace eda::gate::optimizer {

  ConeVisitor::ConeVisitor(const Cut &cut, GNet *sourceNet)
          : cut(cut), sourceNet(sourceNet) {
    subnetId = sourceNet->newSubnet();
  }

  ConeVisitor::~ConeVisitor() {
    // TODO: delete created gnet here?
  }

  VisitorFlags ConeVisitor::onNodeBegin(const GateID &node) {

    // TODO: delete print.
    std::cout << "onNodeBegin " << node << std::endl;

    sourceNet->moveGate(node, subnetId);
    if (cut.find(node) != cut.end()) {
      resultCut.emplace(node);
      return FINISH_THIS;
    }
    return SUCCESS;
  }

  VisitorFlags ConeVisitor::onNodeEnd(const GateID &) {
    return SUCCESS;
  }

  VisitorFlags ConeVisitor::onCut(const Visitor::Cut &) {
    return SUCCESS;
  }

  ConeVisitor::GNet *ConeVisitor::getGNet() {
    return sourceNet->subnets()[subnetId];
  }
} // namespace eda::gate::optimizer