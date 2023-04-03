//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/optimizer.h"

namespace eda::gate::optimizer {

  void optimize(GNet *net, int cutSize) {
    CutStorage cutStorage = findCuts(cutSize, net);

    OptimizerVisitor optimizerVisitor(&cutStorage, net);
    Walker walker(net, &optimizerVisitor, &cutStorage);

    // TODO: change for normal condition.
    while(true) {
      walker.walk(true);
      break;
    }
  }

  void optimizePrint(GNet *net, int cutSize, const std::filesystem::path &subCatalog) {
    CutStorage cutStorage = findCuts(cutSize, net);

    OptimizerVisitor optimizerVisitor(&cutStorage, net);
    TrackerVisitor trackerVisitor(subCatalog, net, &optimizerVisitor);
    Walker walker(net, &trackerVisitor, &cutStorage);

    // TODO: change for normal condition.
    while(true) {
      walker.walk(true);
      break;
    }
  }

  CutStorage findCuts(int cutSize, GNet *net) {
    CutStorage cutStorage;

    CutsFindVisitor visitor(cutSize, &cutStorage);
    Walker firstFind(net, &visitor, &cutStorage);
    // Find cuts on the first iteration.
    firstFind.walk(true);

    return cutStorage;
  }

} // namespace eda::gate::optimizer