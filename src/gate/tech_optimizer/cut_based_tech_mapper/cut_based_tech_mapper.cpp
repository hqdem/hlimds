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
#include "gate/model2/printer/printer.h"
#include "gate/optimizer/cut_storage.h"
#include "gate/tech_optimizer/cut_based_tech_mapper/cut_based_tech_mapper.h"
#include "gate/tech_optimizer/cut_based_tech_mapper/tech_map_visitor.h"

namespace eda::gate::tech_optimizer {

  using Gate = eda::gate::model::Gate;
  using GateIdMap = std::unordered_map<Gate::Id, Gate::Id>;
  using PreBasis = eda::gate::premapper::PreBasis;
  using CutStorage = eda::gate::optimizer::CutStorage;
  
  CutBasedTechMapper::CutBasedTechMapper(const std::string &libertyPath) {
    LibraryCells libraryCells(libertyPath);
    rwdb.linkDB(dbPath);
    rwdb.openDB();

    libraryCells.initializeLibraryRwDatabase(&rwdb, cellTypeMap);
  }

  CutBasedTechMapper::CutBasedTechMapper(
      eda::gate::optimizer::SQLiteRWDatabase &rwdb,  
      std::unordered_map<std::string, CellTypeID> &cellTypeMap) :
    cellTypeMap(cellTypeMap),
    rwdb(rwdb) {}

  GNet *CutBasedTechMapper::techMap(GNet *net, Strategy *strategy, bool aig) {
    try {
      //if (aig) {aigMap(net);}

      CutStorage cutStorage;
      findCuts(net, cutStorage);

      std::unordered_map<GateID, Replacement> bestReplacement;
      replacementSearch(net, strategy, bestReplacement, cutStorage,
          cellTypeMap);

      const Net &model2 = createModel2(net, bestReplacement);
      printNet(model2);

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

  void CutBasedTechMapper::findCuts(GNet *net, CutStorage &cutStorage) {
    cutStorage = eda::gate::optimizer::findCuts(net, 6);
  }

  void CutBasedTechMapper::replacementSearch(GNet *net, Strategy *strategy,  
      std::unordered_map<GateID, Replacement> &bestReplacement, 
      CutStorage &cutStorage,
      std::unordered_map<std::string, CellTypeID> &cellTypeMap) {

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
        rwdb, strategy, cellTypeMap);
    eda::gate::optimizer::CutWalker walker(net, &searchOptReplacement,
        &cutStorage);
    walker.walk(true);
  }

  const Net &CutBasedTechMapper::createModel2(GNet *net, 
      std::unordered_map<GateID, Replacement> &bestReplacement) {

    eda::gate::model::NetBuilder netBuilder;
    std::stack<Replacement*> stack;
    std::unordered_set<Replacement*> visited;
    optimizer::CutWalker::GateIdSet targets;

    targets.reserve((net->targetLinks()).size());

    for (auto link : net->targetLinks()) {
      targets.insert(link.target);
    }

    for (const auto &outGateID: targets) {
      for (const auto &preOutGateID: Gate::get(outGateID)->inputs()) {
        std::cout << "добавляем корень" << preOutGateID.node() << std::endl;
        stack.push(&bestReplacement.at(preOutGateID.node()));
        visited.insert(&bestReplacement.at(preOutGateID.node()));
      }
    }

    while (!stack.empty()) {
      Replacement* current = stack.top();
      std::cout << current->name << std::endl;

      if (current->isInput) {

        std::cout << "добавляем вход: " << current->rootNode << std::endl;
        auto cellID = model::makeCell(eda::gate::model::CellSymbol::IN);
        netBuilder.addCell(cellID);
        current->cellID = cellID;
        current->used = true;
        stack.pop();

      } else {

        bool readyForCreate = true;

        for (auto& [input, secondParam] : current->map) {
          std::cout << secondParam << std::endl;
          if (!bestReplacement.at(secondParam).used) {
            std::cout << "этот не создался: " << secondParam << std::endl;
            readyForCreate = false;
          }
        }

        if (readyForCreate) {
          std::cout << "создаем ячейку" << std::endl;
          current->used = true;
          model::Cell::LinkList linkList;
          for (auto& [input, secondParam] : current->map) {
            linkList.push_back(model::LinkEnd(
                bestReplacement.at(secondParam).cellID));
          }
          auto cellID = makeCell(current->cellType, linkList);
          netBuilder.addCell(cellID);
          current->cellID = cellID;
          stack.pop();
        } else {
          std::cout << "мы не создали входов для этого элемента" << std::endl;
        }
      }


      for (const auto& [input, secondParam] : current->map) {
        std::cout << "пытаемся добавляем потомка: " << secondParam << std::endl;
        if (visited.find(&bestReplacement.at(secondParam)) == visited.end()) {
          std::cout << "добавляем потомка: " << secondParam << std::endl;
          stack.push(&bestReplacement.at(secondParam));
          visited.insert(&bestReplacement.at(secondParam));
        }
      }
    }

    const Net &model2 = Net::get(netBuilder.make());
    std::cout << model2 << std::endl;
    return model2;
  }

  void CutBasedTechMapper::printNet(const Net &model2) {
    // Create an instance of the NetPrinter class for the VERILOG format
    eda::gate::model::NetPrinter& verilogPrinter = eda::gate::model::NetPrinter::getPrinter(eda::gate::model::VERILOG);

    // Open a stream for writing Verilog code to a file or console
    std::ofstream outFile("output.v");  // Or use std::cout to print to the console

    // Call the NetPrinter::print method to generate Verilog code
    verilogPrinter.print(outFile, model2, "my_net");

    // Close the stream
    outFile.close();
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
