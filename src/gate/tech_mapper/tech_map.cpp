//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/optimizer.h"
#include "gate/premapper/aigmapper.h"
#include "gate/tech_mapper/tech_map.h"
#include "gate/tech_mapper/tech_map_visitor.h"
#include "gate/tech_mapper/library/cell.h"

namespace eda::gate::techMap {
  using Gate = eda::gate::model::Gate;
  using GateIdMap = std::unordered_map<Gate::Id, Gate::Id>;
  using PreBasis = eda::gate::premapper::PreBasis;
  using SQLiteRWDatabase = eda::gate::optimizer::SQLiteRWDatabase;
  
  TechMap::TechMap(std::string libertyPath) {
    LibraryCells libraryCells(libertyPath);

    SQLiteRWDatabase arwdb;
    std::string dbPath = "rwtest.db";

    arwdb.linkDB(dbPath);
    arwdb.openDB();

    initializeLibraryRwDatabase(libraryCells.cells, &arwdb);
  }

  GNet *TechMap::techMap(GNet *net) {
    aigMap(net);
    findCuts(net);
    replacementSearch(net);
    replacement(net);
    std::cout << getArea(net) << std::endl;
    std::cout << getDelay(net) << std::endl;
    return net;
  }
  
  void TechMap::aigMap(GNet *net) {
    std::shared_ptr<GNet> sharedNet(net);
    sharedNet->sortTopologically();

    GateIdMap gmap;
    std::shared_ptr<GNet> premapped = 
        eda::gate::premapper::getPreMapper(PreBasis::AIG).map(*sharedNet, gmap);
    premapped->sortTopologically();

    net = premapped.get();
  }

  void TechMap::findCuts(GNet *net) {
    cutStorage = eda::gate::optimizer::findCuts(6, net);
  }

  void TechMap::replacementSearch(GNet *net) {
    SearchOptReplacement searchOptReplacement;
    searchOptReplacement.set(&cutStorage, net, &bestReplacement, 6, rwdb);
    eda::gate::optimizer::TrackerVisitor trackerVisitor("test/data/gate/tech_map/before", 
        net, &searchOptReplacement);
    eda::gate::optimizer::Walker walker(net, &trackerVisitor, &cutStorage);
    walker.walk(true);
  }

  void TechMap::replacement(GNet *net) {
    auto nodes = eda::utils::graph::topologicalSort(*net);
    std::reverse(nodes.begin(), nodes.end());
    for (auto &node: nodes) {
      if (bestReplacement.count(node) & net->hasNode(node)) {
        Replacement &replacementInfo = bestReplacement.at(node);
        eda::gate::optimizer::substitute(node, replacementInfo.bestOptionMap,
            replacementInfo.subsNet, replacementInfo.net);
                
        if (delay < replacementInfo.delay) {delay = replacementInfo.delay;}
        area = area + replacementInfo.area;
      } 
    }
  }

  float TechMap::getArea(GNet *net) {
    return area;
  }
  float TechMap::getDelay(GNet *net) {
    return delay;
  }
}