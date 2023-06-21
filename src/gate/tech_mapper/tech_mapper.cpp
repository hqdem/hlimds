//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/optimizer.h"
#include "gate/tech_mapper/replacement_struct.h"
#include "gate/tech_mapper/strategy/replacement_cut.h"
#include "gate/tech_mapper/tech_mapper.h"

namespace eda::gate::optimizer {

  void techMap(GNet *net, int cutSize, RWDatabase &rwdb, 
      TechMapVisitor&& techMapper, ReplacementVisitor&& replacer) {
    std::unordered_map<GateID, double> gatesDelay;
    std::unordered_map<GateID, Replacement> bestReplacement;

    CutStorage cutStorage = findCuts(cutSize, net);

    techMapper.set(&cutStorage, net, &bestReplacement, cutSize, rwdb);

    TrackerVisitor trackerVisitor("test/data/gate/tech_map/before", net, &techMapper);
    Walker walker(net, &trackerVisitor, &cutStorage);

    while(true) {
      walker.walk(true);
      break;
    }

    if (bestReplacement.size() != 0) {
      double area = 0; 
      double delay = 0;
      replacer.set(&cutStorage, net, &bestReplacement, cutSize, area, delay);

      TrackerVisitor trackerVisitor("test/data/gate/tech_map", net, &replacer);
      Walker repl(net, &trackerVisitor, &cutStorage);

      while(true) {
        repl.walk(false);
        break;
      }
    }

  }

  void
  techMapPrinter(GNet *net, int cutSize, RWDatabase &rwdb, 
      TechMapVisitor&& techMapper, ReplacementVisitor&& replacer,
      const std::filesystem::path &subCatalog) {
    std::unordered_map<GateID, double> gatesDelay;
    std::unordered_map<GateID, Replacement> bestReplacement;

    CutStorage cutStorage = findCuts(cutSize, net);
    techMapper.set(&cutStorage, net, &bestReplacement, cutSize, rwdb);

    TrackerVisitor trackerVisitor(subCatalog / "before", net, &techMapper);
    Walker walker(net, &trackerVisitor, &cutStorage);

    while(true) {
      walker.walk(true);
      break;
    }

    if (bestReplacement.size() != 0) {
      double area = 0; 
      double delay = 0;
      replacer.set(&cutStorage, net, &bestReplacement, cutSize, area, delay);

      TrackerVisitor trackerVisitor(subCatalog, net, &replacer);
      Walker repl(net, &trackerVisitor, &cutStorage);

      while(true) {
        repl.walk(false);
        break;
      }

      //std::cout << "delay: " << delay << std::endl;
      //std::cout << "area: " << area << std::endl;
    }
  }
  
} // namespace eda::gate::optimizer