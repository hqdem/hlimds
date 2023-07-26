
//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/cut_walker.h"
#include "gate/optimizer/optimizer.h"

namespace eda::gate::optimizer {

  void optimize(GNet *net, unsigned int cutSize, OptimizerVisitor &&optimizer,
                unsigned int maxCutsNumber) {
    CutStorage cutStorage = findCuts(net, cutSize, maxCutsNumber);

    std::cout << "cuts found" << std::endl;

    optimizer.set(&cutStorage, net, cutSize, maxCutsNumber);
    CutWalker walker(net, &optimizer, &cutStorage);

    // TODO: change condition.
    while (true) {
      walker.walk(true);
      break;
    }
  }

  void optimizePrint(GNet *net, unsigned int cutSize,
                     const std::filesystem::path &subCatalog,
                     OptimizerVisitor &&optimizer,
                     unsigned int maxCutsNumber) {
    CutStorage cutStorage = findCuts(net, cutSize, maxCutsNumber);

    std::cout << "cuts found" << std::endl;

    optimizer.set(&cutStorage, net, cutSize, maxCutsNumber);
    TrackerVisitor trackerVisitor(subCatalog, net, &optimizer);
    CutWalker walker(net, &trackerVisitor, &cutStorage);

    // TODO: change for normal condition.
    while (true) {
      walker.walk(true);
      break;
    }
  }

  CutStorage findCuts(GNet *net, unsigned int cutSize,
                      unsigned int maxCutsNumber) {
    CutStorage cutStorage;

    CutsFindVisitor visitor(cutSize, &cutStorage, maxCutsNumber);
    Walker firstFind(net, &visitor);
    // Find cuts on the first iteration.
    firstFind.walk(true);

    return cutStorage;
  }

} // namespace eda::gate::optimizer
