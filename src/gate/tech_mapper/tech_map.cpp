//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/optimizer.h"
#include "gate/optimizer/cut_walker.h"
#include "gate/premapper/aigmapper.h"
#include "gate/tech_mapper/library/cell.h"
#include "gate/tech_mapper/tech_map.h"
#include "gate/tech_mapper/tech_map_visitor.h"

namespace eda::gate::techMap {

  using Gate = eda::gate::model::Gate;
  using GateIdMap = std::unordered_map<Gate::Id, Gate::Id>;
  using PreBasis = eda::gate::premapper::PreBasis;
  
  TechMapper::TechMapper(const std::string &libertyPath) {
    LibraryCells libraryCells(libertyPath);

    rwdb.linkDB(dbPath);
    rwdb.openDB();

    libraryCells.initializeLibraryRwDatabase(&rwdb);
  }

  TechMapper::TechMapper(eda::gate::optimizer::SQLiteRWDatabase &rwdb) :
  rwdb(rwdb) {}

  GNet *TechMapper::techMap(GNet *net, Strategy *strategy, bool aig) {
    try {
      if (aig) {aigMap(net);}
      findCuts(net);
      replacementSearch(net, strategy);
      replacement(net);
      std::cout << getArea() << std::endl;
      std::cout << getDelay() << std::endl;
      rwdb.closeDB();
    } catch (const char* msg) {
      std::cout << msg << std::endl;
    }
    remove(dbPath.c_str());
    return net;
  }
  
  void TechMapper::aigMap(GNet *&net) {
    std::shared_ptr<GNet> sharedNet(net);
    sharedNet->sortTopologically();
    GateIdMap gmap;
    std::shared_ptr<GNet> premapped = 
        eda::gate::premapper::getPreMapper(PreBasis::AIG).map(*sharedNet, gmap);
    premapped->sortTopologically();
    net = new GNet(*premapped);
  }

  void TechMapper::findCuts(GNet *net) {
    cutStorage = eda::gate::optimizer::findCuts(net, 6);
  }

  void TechMapper::replacementSearch(GNet *net, Strategy *strategy) {
    SearchOptReplacement searchOptReplacement;
    searchOptReplacement.set(&cutStorage, net, &bestReplacement, 6,
        rwdb, strategy);
    eda::gate::optimizer::CutWalker walker(net, &searchOptReplacement,
        &cutStorage);
    walker.walk(true);
  }

  void TechMapper::replacement(GNet *net) {
    auto nodes = eda::utils::graph::topologicalSort(*net);
    std::reverse(nodes.begin(), nodes.end());
    for (auto &node: nodes) {
      if (bestReplacement.count(node) & net->hasNode(node)) {
        Replacement &replacementInfo = bestReplacement.at(node);
        replacementInfo.netSubstitute.substitute();
                
        if (delay < replacementInfo.delay) {delay = replacementInfo.delay;}
        area = area + replacementInfo.area;
      } 
    }
  }

  float TechMapper::getArea() const{
    return area;
  }
  float TechMapper::getDelay() const{
    return delay;
  }
} // namespace eda::gate::techMap
