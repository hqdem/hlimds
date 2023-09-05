//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/strategy/associative_balancer.h"

namespace eda::gate::optimizer {
  AssociativeBalancer::AssociativeBalancer(GNet *net) : net(net) {}

  void AssociativeBalancer::balance() {
    DepthFindVisitor::GateDMap gateDepth;
    DepthFindVisitor depthFindVisitor(gateDepth, net);
    Walker depthFindWalker(net, &depthFindVisitor);
    depthFindWalker.walk(true);

    AssocBalanceVisitor balanceVisitor(net, gateDepth);
    Walker walker(net, &balanceVisitor);

    walker.walk(true);
    balancesNumber = balanceVisitor.getBalancesNumber();
  }

  int AssociativeBalancer::getBalancesNumber() const {
    return balancesNumber;
  }
} // namespace eda::gate::optimizer
