//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/celltype.h"
#include "gate/model2/printer/printer.h"
#include "gate/optimizer/cut_storage.h"
#include "gate/optimizer/cut_walker.h"
#include "gate/optimizer/net_substitute.h"
#include "gate/optimizer/optimizer.h"
#include "gate/premapper/aigmapper.h"
#include "gate/techmapper/cut_based_tech_mapper/cut_based_tech_mapper.h"
#include "gate/techmapper/cut_based_tech_mapper/tech_map_visitor.h"
#include "gate/techmapper/library/cell.h"

namespace eda::gate::techmapper {

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
  rwdb(rwdb),
  cellTypeMap(cellTypeMap) {}

GNet *CutBasedTechMapper::techMap(GNet *net, Strategy *strategy, bool aig) {
  try {
    if (aig) {aigMap(net);}

    CutStorage cutStorage;
    findCuts(net, cutStorage);

    std::unordered_map<GateID, Replacement> bestSubstitutions;
    replacementSearch(net, strategy, bestSubstitutions, cutStorage,
        cellTypeMap);

    const Net &model2 = createModel2(net, bestSubstitutions);
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
    std::unordered_map<GateID, Replacement> &bestSubstitutions, 
    CutStorage &cutStorage,
    std::unordered_map<std::string, CellTypeID> &cellTypeMap) {

  auto nodes = eda::utils::graph::topologicalSort(*net);
  for (const auto &id: net->getSources()) {
    if (Gate::get(id)->isSource()) {
      Replacement bestReplacment{id, eda::gate::model::CELL_TYPE_ID_IN, 
          " ", 0, 0};
      bestSubstitutions.insert(std::pair<GateID, Replacement>
                              (id, bestReplacment));
    }
  }

  SearchOptReplacement searchOptReplacement;
  searchOptReplacement.set(&cutStorage, net, &bestSubstitutions, 6,
      rwdb, strategy, cellTypeMap);
  eda::gate::optimizer::CutWalker walker(net, &searchOptReplacement,
      &cutStorage);
  walker.walk(true);
}

const Net &CutBasedTechMapper::createModel2(GNet *net, 
    std::unordered_map<GateID, Replacement> &bestSubstitutions) {

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
      stack.push(&bestSubstitutions.at(preOutGateID.node()));
      visited.insert(&bestSubstitutions.at(preOutGateID.node()));
    }
  }

  while (!stack.empty()) {
    Replacement* current = stack.top();

    if (current->isInput) {

      auto cellID = model::makeCell(eda::gate::model::CellSymbol::IN);
      netBuilder.addCell(cellID);
      current->cellID = cellID;
      current->used = true;
      stack.pop();

    } else {

      bool readyForCreate = true;

      for (auto& [input, secondParam] : current->map) {
        if (!bestSubstitutions.at(secondParam).used) {
          readyForCreate = false;
        }
      }

      if (readyForCreate) {
        current->used = true;
        model::Cell::LinkList linkList;
        for (auto& [input, secondParam] : current->map) {
          linkList.push_back(model::LinkEnd(
              bestSubstitutions.at(secondParam).cellID));
        }
        auto cellID = makeCell(current->cellType, linkList);
        netBuilder.addCell(cellID);
        current->cellID = cellID;
        stack.pop();
      }
    }


    for (const auto& [input, secondParam] : current->map) {
      if (visited.find(&bestSubstitutions.at(secondParam)) == visited.end()) {
        stack.push(&bestSubstitutions.at(secondParam));
        visited.insert(&bestSubstitutions.at(secondParam));
      }
    }
  }

  const Net &model2 = Net::get(netBuilder.make());
  std::cout << model2 << std::endl;
  return model2;
}

void CutBasedTechMapper::printNet(const Net &model2) {
  // Create an instance of the NetPrinter class for the VERILOG format
  eda::gate::model::ModelPrinter& verilogPrinter =
      eda::gate::model::ModelPrinter::getPrinter(eda::gate::model::ModelPrinter::VERILOG);

  // Open a stream for writing Verilog code to a file or console
  std::ofstream outFile("output.v");  // Or use std::cout to print to the console

  // Call the NetPrinter::print method to generate Verilog code
  verilogPrinter.print(outFile, model2, "my_net");

  // Close the stream
  outFile.close();
}

float CutBasedTechMapper::getArea() const{
  return area;
}

float CutBasedTechMapper::getDelay() const{
  return delay;
}

} // namespace eda::gate::techmapper
