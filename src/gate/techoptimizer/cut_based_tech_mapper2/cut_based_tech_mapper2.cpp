//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#include "gate/model2/celltype.h"
#include "gate/model2/printer/printer.h"
#include "gate/premapper/aigmapper.h"
#include "gate/techoptimizer/cut_based_tech_mapper2/cut_based_tech_mapper.h"
#include "gate/techoptimizer/cut_based_tech_mapper/tech_map_visitor.h"
#include "gate/techoptimizer/library/cell.h"

namespace eda::gate::tech_optimizer {

  using Gate = eda::gate::model::Gate;
  using GateIdMap = std::unordered_map<Gate::Id, Gate::Id>;
  using PreBasis = eda::gate::premapper::PreBasis;
  using CutStorage = eda::gate::optimizer::CutStorage;
  

  void CutBasedTechMapper2::set(CellDB cellDB, Strategy2 *strategy) {
    this->cellDB = cellDB;
    this->strategy = strategy;
  }

  SubnetID CutBasedTechMapper2::techMap(SubnetID subnetID) {
      
      // TODO
      if (false) {
        aigMap(net);
      }

      auto cutExtractor = findCuts(subnetID);

      auto bestReplacementMap = replacementSearch(subnetID, cutExtractor);

      const SubnetID mappedSubnet = createTechMappedSubnet(bestReplacementMap);

    return net;
  }
  
  void CutBasedTechMapper2::aigMap(SubnetID subnetID) {
  }

  CutExtractor CutBasedTechMapper2::findCuts(SubnetID subnetID) {
    // 6 - max of technology cells input
    CutExtractor cutExtractor(Subnet::get(subnetID), 6);
    return cutExtractor;
  }

  std::map<CellID, BestReplacement> CutBasedTechMapper2::replacementSearch(
    SubnetID subnetID, CutExtractor &cutExtractor) {
    std::map<CellID, BestReplacement> bestReplacementMap;
    Subnet subnet = Subnet::get(subnetID);

    Array<Entry> entries = subnet.getEntries();

    for (const auto &entry : subnet.getEntries()) {
      auto cell = entry.cell;

      if (cell.isIn()) {

      } else if (cell.isOut()) {

      } else if (cell.isZero()) {
        
      } else if (cell.isOne()) {
        
      } 

      auto cellID = cell.cell;

      // Save best tech cells subnet to bestReplMap 
      strategy.findBest(cellID, cutExtractor.getCuts(cellID), 
          bestReplacementMap, cellDB.ttSubnetMap);
    }
    return bestReplacementMap;
  }

  SubnetID CutBasedTechMapper::buildSubnet(std::map<CellID, BestReplacement> &bestReplacementMap) {
    eda::gate::model::SubnetBuilder subnetBuilder;

    std::stack<CellID> stack;
    std::unordered_set<CellID> visited;

    for (const auto& pair : bestReplacementMap) {
      CellID cellID = pair.first;

      if (Cell::get(cellID).isOut()) {
        stack.push(cellID);
        visited.insert(cellID);
      } 
    }
  }

  const Net &CutBasedTechMapper::buildModel2(std::map<CellID, BestReplacement> &bestReplacementMap) {

    eda::gate::model::NetBuilder netBuilder;

    std::stack<CellID> stack;
    std::unordered_set<CellID> visited;

    for (const auto& pair : bestReplacementMap) {
      CellID cellID = pair.first;

      if (Cell::get(cellID).isOut()) {
        stack.push(cellID);
        visited.insert(cellID);
      } 
    }

    while (!stack.empty()) {
      cellID current = stack.top();

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
    eda::gate::model::NetPrinter& verilogPrinter = eda::gate::model::NetPrinter::getPrinter(eda::gate::model::VERILOG);

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
} // namespace eda::gate::tech_optimizer
