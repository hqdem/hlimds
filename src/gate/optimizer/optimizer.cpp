//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/optimizer.h"

using Cut = CutStorage::Cut;

Optimizer::Optimizer(Optimizer::GNet *gNet) : gNet(gNet) {}

void Optimizer::optimize(int cutSize) {
  const auto& sources = gNet->getSources();

  for(const auto& source : sources) {
    Cut cut;

    cut.emplace(source);
    cutStorage.cuts[source].emplace(cut);
  }

  CutsFindVisitor visitor(cutSize, &cutStorage);
  Walker firstFind(gNet, &visitor, &cutStorage);
  // Find cuts on the first iteration.
  firstFind.walk();
}
