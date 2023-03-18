//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/walker.h"

Walker::Walker(Walker::GNet *gNet, Visitor *visitor, CutStorage *cutStorage) :
        gNet(gNet), visitor(visitor), cutStorage(cutStorage) {}

void Walker::walk(bool forward) {

  auto nodes = eda::utils::graph::topologicalSort(*gNet);
  if (!forward) {
    std::reverse(nodes.begin(), nodes.end());
  }

  for (auto &node: nodes) {
    visitor->onNodeBegin(node);
    auto &cuts = cutStorage->cuts[node];
    for (const auto &cut: cuts) {
      visitor->onCut(cut);
    }
  }
}
