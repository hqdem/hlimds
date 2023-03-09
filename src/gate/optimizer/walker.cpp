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

void Walker::walk() {
  auto nodes = eda::utils::graph::topologicalSort(*gNet);
  for (auto &node: nodes) {
    visitor->onGate(node);
    auto &cuts = cutStorage->cuts[node];
    for (const auto &cut: cuts) {
      visitor->onCut(cut);
    }
  }
}
