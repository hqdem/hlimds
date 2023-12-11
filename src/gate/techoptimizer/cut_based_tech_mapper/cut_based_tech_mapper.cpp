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
#include "gate/techoptimizer/cut_based_tech_mapper/cut_based_tech_mapper.h"

#include <limits.h>

namespace eda::gate::tech_optimizer {

  using Subnet = model::Subnet;

  std::vector<EntryIndex> outID;

  void CutBasedTechMapper::set(CellDB &cellDB, Strategy *strategy) {
    this->cellDB = cellDB;
    this->strategy = strategy;
  }

  SubnetID CutBasedTechMapper::techMap(SubnetID subnetID) {
      
      // TODO
      if (false) {
        //aigMap(net);
      }

      auto cutExtractor = findCuts(subnetID);

      auto bestReplacementMap = replacementSearch(subnetID, cutExtractor);

      const SubnetID mappedSubnet = buildSubnet(subnetID, bestReplacementMap);

    return mappedSubnet;
  }
  
  void CutBasedTechMapper::aigMap(SubnetID subnetID) {
  }

  CutExtractor CutBasedTechMapper::findCuts(SubnetID subnetID) {
    // 6 - max of technology cells input
    CutExtractor cutExtractor(model::Subnet::get(subnetID), 6);
    return cutExtractor;
  }

  std::map<EntryIndex, BestReplacement> CutBasedTechMapper::replacementSearch(
      SubnetID subnetID, CutExtractor &cutExtractor) {

    std::map<EntryIndex, BestReplacement> bestReplacementMap;
    Subnet subnet = Subnet::get(subnetID);

    eda::gate::model::Array<Subnet::Entry> enstries = subnet.getEntries();
    for (uint64_t enstryID = 0; i < std::size(enstries); enstryID++) {
      auto cell = enstries[enstryID].cell;

      // TODO: Копипаста, переделать. 
      if (cell.isIn()) {
        BestReplacement bestReplacement;
        bestReplacement.isIN = true;
        bestReplacementMap[enstryID] = bestReplacement;

      } else if (cell.isOut()) {
        outID.push_back(enstryID);

        //BestReplacement bestReplacement;
        //bestReplacement.isOUT = true;
        //bestReplacementMap[enstryID] = bestReplacement;
      } 

      // Save best tech cells subnet to bestReplMap 
      strategy->findBest(entryIndex, cutExtractor.getCuts(entryIndex), 
          bestReplacementMap, cellDB);

      enstryID += cell.more;
    }
    return bestReplacementMap;
  }

  SubnetID CutBasedTechMapper::buildSubnet(SubnetID subnetID, std::map<EntryIndex, BestReplacement> &bestReplacementMap) {
    Subnet subnet = Subnet::get(subnetID);
    eda::gate::model::Array<Subnet::Entry> enstries = subnet.getEntries();
    
    eda::gate::model::SubnetBuilder subnetBuilder;

    std::stack<EntryIndex> stack;
    std::unordered_set<EntryIndex> visited;

    for (const auto& out : outID) {
      stack.push(out);
      visited.insert(out);
    }

    while (!stack.empty()) {
      EntryIndex currentEntryIDX = stack.top();
      auto currentCell = enstries[currentEntryIDX].cell;

      if (currentCell.isIn()) {
        auto cellID = subnetBuilder.addCell(eda::gate::model::CellSymbol::IN);
        bestReplacementMap[currentEntryIDX].cellIDInMappedSubnet = cellID;
        stack.pop();

      } else {
        bool readyForCreate = true;

        for (EntryIndex entryIDX = currentEntryIDX + 1; 
            entryIDX <= currentEntryIDX + currentCell.more; entryIDX++) {
          if (bestReplacementMap[entryIDX] = ULLONG_MAX) {
            readyForCreate = false;
          }
        }

        //Продолжить

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

    return subnetBuilder.make();
  }

  //const Net &CutBasedTechMapper::buildModel2(std::map<CellID, BestReplacement> &bestReplacementMap) {
    /*
    eda::gate::model::NetBuilder netBuilder;

    std::stack<EntryIndex> stack;
    std::unordered_set<EntryIndex> visited;

    for (const auto& pair : bestReplacementMap) {
      EntryIndex entryIndex = pair.first;

      if (entryIndex.isOut()) {
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
    
  //}

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
  */

  float CutBasedTechMapper::getArea() const{
    return area;
  }
  float CutBasedTechMapper::getDelay() const{
    return delay;
  }
} // namespace eda::gate::tech_optimizer
