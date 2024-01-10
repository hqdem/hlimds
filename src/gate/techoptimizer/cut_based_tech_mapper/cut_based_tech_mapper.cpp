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
#include "gate/transformer/aigmapper.h"

#include <limits.h>

namespace eda::gate::tech_optimizer {

  using Subnet = model::Subnet;

  std::vector<EntryIndex> outID;

  CutBasedTechMapper::CutBasedTechMapper(CellDB &cellDB) {
    this->cellDB = cellDB;
  }

  void CutBasedTechMapper::setStrategy(Strategy *strategy) {
    this->strategy = strategy;
  }

  SubnetID CutBasedTechMapper::techMap(SubnetID subnetID) {
    transformer::AigMapper mapper;
    const auto transformedSub  = mapper.transform(subnetID);

    auto cutExtractor = findCuts(transformedSub);

    auto bestReplacementMap = replacementSearch(transformedSub, cutExtractor);

    const SubnetID mappedSubnet = buildSubnet(transformedSub, bestReplacementMap);

    return mappedSubnet;
  }
  
  void CutBasedTechMapper::aigMap(SubnetID subnetID) {
  }

  CutExtractor CutBasedTechMapper::findCuts(SubnetID subnetID) {
    // 6 - max of technology cells input
    return CutExtractor(&model::Subnet::get(subnetID), 6);
  }

  std::map<EntryIndex, BestReplacement> CutBasedTechMapper::replacementSearch(
      SubnetID subnetID, CutExtractor &cutExtractor) {

    std::map<EntryIndex, BestReplacement> bestReplacementMap;
    Subnet &subnet = Subnet::get(subnetID);

    eda::gate::model::Array<Subnet::Entry> entries = subnet.getEntries();
    for (uint64_t entryIndex = 0; entryIndex < std::size(entries); 
        entryIndex++) {
      auto cell = entries[entryIndex].cell;

      if (cell.isIn()) {
        BestReplacement bestReplacement{true};
        bestReplacementMap[entryIndex] = bestReplacement;

      } else if (cell.isOut()) {
        outID.push_back(entryIndex);

        BestReplacement bestReplacement{false, true};
        bestReplacement.entryIDxs.insert(cell.link[0].idx);
        bestReplacementMap[entryIndex] = bestReplacement;
      } else {
        // Save best tech cells subnet to bestReplMap 
        strategy->findBest(entryIndex, cutExtractor.getCuts(entryIndex), 
            bestReplacementMap, cellDB, subnetID);
      }

      entryIndex += cell.more;
    }
    return bestReplacementMap;
  }

  SubnetID CutBasedTechMapper::buildSubnet(SubnetID subnetID, std::map<EntryIndex, BestReplacement> &bestReplacementMap) {
    Subnet &subnet = Subnet::get(subnetID);
    eda::gate::model::Array<Subnet::Entry> entries = subnet.getEntries();
    
    eda::gate::model::SubnetBuilder subnetBuilder;

    std::stack<EntryIndex> stack;
    std::unordered_set<EntryIndex> visited;


    for (const auto& out : outID) {
      stack.push(out);
      visited.insert(out);
    }

    while (!stack.empty()) {
      EntryIndex currentEntryIDX = stack.top();
      auto currentCell = entries[currentEntryIDX].cell;

      if (currentCell.isIn()) {
        auto cellID = subnetBuilder.addCell(eda::gate::model::CellSymbol::IN);
        bestReplacementMap[currentEntryIDX].cellIDInMappedSubnet = cellID;
        stack.pop();

      } else {
        bool readyForCreate = true;
        for (const auto &idx : bestReplacementMap.at(currentEntryIDX).entryIDxs) {
          if (bestReplacementMap[idx].cellIDInMappedSubnet == ULLONG_MAX) {
            readyForCreate = false;
            break;
          }
        }

        if (readyForCreate) {
          if (currentCell.isOut()) {
            auto cellID = subnetBuilder.addCell(
                eda::gate::model::CellSymbol::OUT,
                Subnet::Link(currentCell.link[0].idx));
            bestReplacementMap[currentEntryIDX].cellIDInMappedSubnet = cellID;
            stack.pop();
          } else {
            Subnet::LinkList linkList;

            for (const auto &idx : bestReplacementMap.at(currentEntryIDX).entryIDxs) {
              Subnet::Link link(idx);
              linkList.push_back(link);
            }

            Subnet &techSubnet = Subnet::get(bestReplacementMap[currentEntryIDX].subnetID);
            eda::gate::model::Array<Subnet::Entry> techCellEntries = techSubnet.getEntries();
            for (const auto &techCellEntry : techCellEntries) {
              auto techCell = techCellEntry.cell;
              if (!techCell.isIn() && !techCell.isOut()) {
                auto cellID = subnetBuilder.addCell(techCell.getTypeID(), linkList);
                bestReplacementMap[currentEntryIDX].cellIDInMappedSubnet = cellID;
              }
            }

            // when addSubnet() ready uncomment this and delete upper
            //auto cellID = subnetBuilder.addSubnet(
            //    bestReplacementMap[currentEntryIDX].subnetID, linkList);
            //bestReplacementMap[currentEntryIDX].cellIDInMappedSubnet = cellID;

            stack.pop();
          }
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
