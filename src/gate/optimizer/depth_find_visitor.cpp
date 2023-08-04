//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "depth_find_visitor.h"

namespace eda::gate::optimizer {

  DepthFindVisitor::DepthFindVisitor(GateDMap &gateDepth, const GNet *net) :
                                     gateDepth(gateDepth),
                                     net(net) {}

  int DepthFindVisitor::getNetDepth() const {
    int depth = 0;
    for (const auto &elem : gateDepth) {
      depth = std::max(depth, elem.second);
    }
    return depth;
  }

  VisitorFlags DepthFindVisitor::onNodeBegin(const GateID &gateId) {
    if (gateDepth.find(gateId) == gateDepth.end()) {
      gateDepth[gateId] = 0;
    }
      
    for (const auto& link : net->getOutEdges(gateId)) {
      GateID outGate = net->leadsTo(link);
      gateDepth[outGate] = std::max(gateDepth[gateId] + 1, gateDepth[outGate]);
    }
    return VisitorFlags::CONTINUE;
  }

  VisitorFlags DepthFindVisitor::onNodeEnd(const GateID &gateId) {
    return VisitorFlags::CONTINUE;
  }
} // namespace eda::gate::optimizer
