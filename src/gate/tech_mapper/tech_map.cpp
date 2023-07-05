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
  
  TechMapper::TechMapper(std::string libertyPath) {
    LibraryCells libraryCells(libertyPath);

    rwdb.linkDB(dbPath);
    rwdb.openDB();

    libraryCells.initializeLibraryRwDatabase(&rwdb);

    for (auto const [TT,BD] : rwdb._storage) {
      std::cout << TT << std::endl;
    }
  }

  GNet *TechMapper::techMap(GNet *net, Strategy &strategy) {
    //aigMap(net);
    findCuts(net);
    replacementSearch(net, strategy);
    replacement(net);
    std::cout << getArea(net) << std::endl;
    std::cout << getDelay(net) << std::endl;

    rwdb.closeDB();
    remove(dbPath.c_str());
    return net;
  }
  
  void TechMapper::aigMap(GNet *net) {
    std::shared_ptr<GNet> sharedNet(net);
    sharedNet->sortTopologically();

    GateIdMap gmap;
    std::shared_ptr<GNet> premapped = 
        eda::gate::premapper::getPreMapper(PreBasis::AIG).map(*sharedNet, gmap);

    net = premapped.get();
  }

  void TechMapper::findCuts(GNet *net) {
    cutStorage = eda::gate::optimizer::findCuts(6, net);
  }

  void TechMapper::replacementSearch(GNet *net, Strategy &strategy) {
    SearchOptReplacement searchOptReplacement;
    searchOptReplacement.set(&cutStorage, net, &bestReplacement, 6, rwdb, strategy);
    eda::gate::optimizer::Walker walker(net, &searchOptReplacement, &cutStorage);
    walker.walk(true);
  }

  void TechMapper::replacement(GNet *net) {
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

  float TechMapper::getArea(GNet *net) {
    return area;
  }
  float TechMapper::getDelay(GNet *net) {
    return delay;
  }
}