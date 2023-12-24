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
      //if () {
        //aigMap(net);
      //}

      auto cutExtractor = findCuts(subnetID);

      auto bestReplacementMap = replacementSearch(subnetID, cutExtractor);

      const SubnetID mappedSubnet = buildSubnet(subnetID, bestReplacementMap);

    return mappedSubnet;
  }
  
  void CutBasedTechMapper::aigMap(SubnetID subnetID) {
  }

  CutExtractor CutBasedTechMapper::findCuts(SubnetID subnetID) {
    // 6 - max of technology cells input
    CutExtractor cutExtractor(&model::Subnet::get(subnetID), 6);
    return cutExtractor;
  }

  std::map<EntryIndex, BestReplacement> CutBasedTechMapper::replacementSearch(
      SubnetID subnetID, CutExtractor &cutExtractor) {

    std::map<EntryIndex, BestReplacement> bestReplacementMap;
    Subnet subnet = Subnet::get(subnetID);

    eda::gate::model::Array<Subnet::Entry> entries = subnet.getEntries();
    for (uint64_t entryIndex = 0; entryIndex < std::size(entries); 
        entryIndex++) {
      auto cell = entries[entryIndex].cell;

      if (cell.isIn()) {
        BestReplacement bestReplacement;
        bestReplacement.isIN = true;
        bestReplacementMap[entryIndex] = bestReplacement;

      } else if (cell.isOut()) {
        outID.push_back(entryIndex);
      } else {
        // Save best tech cells subnet to bestReplMap 
        strategy->findBest(entryIndex, cutExtractor.getCuts(entryIndex), 
            bestReplacementMap, cellDB, entries);
      }

      entryIndex += cell.more;
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
        for (const auto &link : currentCell.link) {
          if (bestReplacementMap[link.idx].cellIDInMappedSubnet == ULLONG_MAX) {
            readyForCreate = false;
          }
        }

        if (readyForCreate) {
          Subnet::LinkList linkList;

          for (const auto &currentLink : currentCell.link) {
            Subnet::Link link(currentLink);
            linkList.push_back(link);
          }

          size_t cellID;
          Subnet techSubnet = Subnet::get(bestReplacementMap[currentEntryIDX].subnetID);
          eda::gate::model::Array<Subnet::Entry> techCellEntries = techSubnet.getEntries();
          for (const auto &techCellEntry : techCellEntries) {
            auto techCell = techCellEntry.cell;
            if (!techCell.isIn() & !techCell.isOut()) {
              cellID = subnetBuilder.addCell(techCell.getSymbol());
            }
          }
          bestReplacementMap[currentEntryIDX].cellIDInMappedSubnet = cellID;

          // when addSubnet() ready uncoment this and delete upper
          //auto cellID = subnetBuilder.addSubnet(
          //    bestReplacementMap[currentEntryIDX].subnetID, linkList);
          //bestReplacementMap[currentEntryIDX].cellIDInMappedSubnet = cellID;

          stack.pop();
        }
      }

      for (const auto &link : currentCell.link) {
        if (visited.find(link.idx) == visited.end()) {
          stack.push(link.idx);
          visited.insert(link.idx);
        }
      }
    }

    return subnetBuilder.make();
  }

  float CutBasedTechMapper::getArea() const{
    return area;
  }
  float CutBasedTechMapper::getDelay() const{
    return delay;
  }
} // namespace eda::gate::tech_optimizer
