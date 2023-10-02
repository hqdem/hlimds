//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/optimizer.h"
#include "gate/optimizer/cut_walker.h"
#include "gate/optimizer/net_substitute.h"
#include "gate/premapper/aigmapper.h"
#include "gate/tech_optimizer/library/cell.h"
#include "gate/model2/celltype.h"

#include "gate/tech_optimizer/cut_based_tech_mapper/cut_based_tech_mapper.h"
#include "gate/tech_optimizer/cut_based_tech_mapper/tech_map_visitor.h"

namespace eda::gate::tech_optimizer {

  using Gate = eda::gate::model::Gate;
  using GateIdMap = std::unordered_map<Gate::Id, Gate::Id>;
  using PreBasis = eda::gate::premapper::PreBasis;
  
  CutBasedTechMapper::CutBasedTechMapper(const std::string &libertyPath) {
    LibraryCells libraryCells(libertyPath);
    rwdb.linkDB(dbPath);
    rwdb.openDB();

    libraryCells.initializeLibraryRwDatabase(&rwdb);
  }

  CutBasedTechMapper::CutBasedTechMapper(eda::gate::optimizer::SQLiteRWDatabase &rwdb) :
  rwdb(rwdb) {}

  GNet *CutBasedTechMapper::techMap(GNet *net, Strategy *strategy, bool aig) {
    try {
      //if (aig) {aigMap(net);}
      findCuts(net);
      replacementSearch(net, strategy);
      std::cout << "переход к созданию новой схемы" << std::endl;
      traversalNode(net);
      //replacement(net);
      std::cout << getArea() << std::endl;
      std::cout << getDelay() << std::endl;
      rwdb.closeDB();
    } catch (const char* msg) {
      std::cout << msg << std::endl;
    }
    remove(dbPath.c_str());
    return net;
  }
  
  void CutBasedTechMapper::aigMap(GNet *&net) {
    std::shared_ptr<GNet> sharedNet(net);
    sharedNet->sortTopologically();
    GateIdMap gmap;
    std::shared_ptr<GNet> premapped = 
        eda::gate::premapper::getPreMapper(PreBasis::AIG).map(*sharedNet, gmap);
    premapped->sortTopologically();
    net = new GNet(*premapped);
  }

  void CutBasedTechMapper::findCuts(GNet *net) {
    cutStorage = eda::gate::optimizer::findCuts(net, 6);
  }

  void CutBasedTechMapper::replacementSearch(GNet *net, Strategy *strategy) {
    auto nodes = eda::utils::graph::topologicalSort(*net);
    for (auto &id: net->getSources()) {
      if (Gate::get(id)->isSource()) {
        Replacement bestReplacment{id, eda::gate::model::CELL_TYPE_ID_IN, 
            " ", 0, 0};
        bestReplacement.insert(std::pair<GateID, Replacement>
                                (id, bestReplacment));
      }
    }
    SearchOptReplacement searchOptReplacement;
    searchOptReplacement.set(&cutStorage, net, &bestReplacement, 6,
        rwdb, strategy);
    eda::gate::optimizer::CutWalker walker(net, &searchOptReplacement,
        &cutStorage);
    walker.walk(true);
  }

  void CutBasedTechMapper::replacement(GNet *net) {
    /*auto nodes = eda::utils::graph::topologicalSort(*net);
    std::reverse(nodes.begin(), nodes.end());
    for (auto &node: nodes) {
      if (bestReplacement.count(node) & net->hasNode(node)) {
        Replacement &replacementInfo = bestReplacement.at(node);
        replacementInfo.netSubstitute.substitute();
                
        if (delay < replacementInfo.delay) {delay = replacementInfo.delay;}
        area = area + replacementInfo.area;
      } 
    }
    */
  }

  void CutBasedTechMapper::mapedNet(GNet *net) {
    

    //NetBuilder netBuilder;

    //auto nodes = eda::utils::graph::topologicalSort(*net);
    //for (auto &node: nodes) {
    //  node.
    //}

  }

  void CutBasedTechMapper::traversalNode(GNet *net) {
    std::stack<Replacement*> stack;
    std::unordered_set<Replacement*> visited;
    bool canPop = false;

    optimizer::CutWalker::GateIdSet targets;
    targets.reserve((net->targetLinks()).size());

    for (auto link : net->targetLinks()) {
      targets.insert(link.target);
    }
    std::cout << "вытаемся сохранить выходы" << std::endl;

    for (const GateID &id: targets) {
      stack.push(&bestReplacement.at(id));
      visited.insert(&bestReplacement.at(id));
    }

    while (!stack.empty()) {
      std::cout << "создаем схему" << std::endl;
      Replacement* current = stack.top();

      if (current->map == nullptr) {
        canPop = true;
        auto cellID = model::makeCell(eda::gate::model::CellSymbol::IN);
        netBuilder.addCell(cellID);
        current->cellID = cellID;
      }

      if (canPop & current->map != nullptr) {
        current->used = true;
        stack.pop();
        model::Cell::LinkList linkList;
        for (auto& [input, secondParam] : *current->map) {
          linkList.push_back(model::LinkEnd(bestReplacement.at(secondParam).cellID));
        }
        auto cellID = makeCell(current->cellType, linkList);
        current->cellID = cellID;
        //mappedNet.createCell();
      }

      for (const auto& [input, secondParam] : *current->map) {
        if (visited.find(&bestReplacement.at(secondParam)) == visited.end()) {
          stack.push(&bestReplacement.at(secondParam));
          visited.insert(&bestReplacement.at(secondParam  ));
        }
      }
    }

  }


  std::vector<Gate::Id> CutBasedTechMapper::getOutputs(GNet *net) {
    //TODO create
    std::vector<Gate::Id> outputs;
    return outputs;
  }

  float CutBasedTechMapper::getArea() const{
    return area;
  }
  float CutBasedTechMapper::getDelay() const{
    return delay;
  }
} // namespace eda::gate::tech_optimizer
