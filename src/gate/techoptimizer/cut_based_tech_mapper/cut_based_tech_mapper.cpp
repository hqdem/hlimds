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

const int MAX_CUT_SIZE = 6;

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

    auto startAIG = std::chrono::high_resolution_clock::now();
    transformer::AigMapper mapper;
    const auto transformedSub  = mapper.transform(subnetID);
    auto endAIG = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> AIGTime = endAIG - startAIG;
    std::cout << "Функция AIG выполнялась " << AIGTime.count() << " секунд.\n";
    std::cout << Subnet::get(transformedSub)<< "\n";

    auto startCut = std::chrono::high_resolution_clock::now();
    CutExtractor cutExtractor(&model::Subnet::get(transformedSub), MAX_CUT_SIZE);
    auto endCut = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> CutFindTime = endCut - startCut;
    std::cout << "Функция CutExtractor выполнялась " << CutFindTime.count() << " секунд.\n";

    auto startFindBestRepl = std::chrono::high_resolution_clock::now();
    auto bestReplacementMap = replacementSearch(transformedSub, cutExtractor);
    auto endFindBestRepl = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> CutFindBestReplTime = endFindBestRepl - startFindBestRepl;
    std::cout << "Функция FindBestRepl выполнялась " << CutFindBestReplTime.count() << " секунд.\n";

    auto startMapped = std::chrono::high_resolution_clock::now();
    const SubnetID mappedSubnet = buildSubnet(transformedSub, bestReplacementMap);
    auto endMapped = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> CutMappedTime = endMapped - startMapped;
    std::cout << "Функция Mapped выполнялась " << CutMappedTime.count() << " секунд.\n";

    std::cout << "Функция AIG выполнялась " << AIGTime.count() << " секунд.\n";
    std::cout << "Функция CutExtractor выполнялась " << CutFindTime.count() << " секунд.\n";
    std::cout << "Функция FindBestRepl выполнялась " << CutFindBestReplTime.count() << " секунд.\n";
    std::cout << "Функция Mapped выполнялась " << CutMappedTime.count() << " секунд.\n";

    return mappedSubnet;
  }

/*SubnetID CutBasedTechMapper::techMap(SubnetID subnetID) {
  transformer::AigMapper mapper;
  const auto transformedSub  = mapper.transform(subnetID);

  CutExtractor cutExtractor(&model::Subnet::get(transformedSub), MAX_CUT_SIZE);
  auto bestReplacementMap = replacementSearch(transformedSub, cutExtractor);

  const SubnetID mappedSubnet = buildSubnet(transformedSub, bestReplacementMap);

  return mappedSubnet;
}*/
  
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

      } else if (cell.isOne()) {
        BestReplacement bestReplacement{};
        bestReplacement.isOne = true;
        bestReplacementMap[entryIndex] = bestReplacement;
      } else if (cell.isZero()) {
        BestReplacement bestReplacement{true};
        bestReplacement.isZero = true;
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
    int countOfCell = 0;

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
        auto cellID = subnetBuilder.addInput();
        bestReplacementMap[currentEntryIDX].cellIDInMappedSubnet = cellID;
        stack.pop();
      } else if (currentCell.isZero()) {
          auto cellID = subnetBuilder.addCell(eda::gate::model::CellSymbol::ZERO);
          bestReplacementMap[currentEntryIDX].cellIDInMappedSubnet = cellID;
          stack.pop();
      } else if (currentCell.isOne()) {
          auto cellID = subnetBuilder.addCell(eda::gate::model::CellSymbol::ONE);
          bestReplacementMap[currentEntryIDX].cellIDInMappedSubnet = cellID;
          stack.pop();
      } else {
        bool readyForCreate = true;
        Subnet::LinkList linkList;

        for (const auto &idx : bestReplacementMap.at(currentEntryIDX).entryIDxs) {
          if (bestReplacementMap[idx].cellIDInMappedSubnet == ULLONG_MAX) {
            readyForCreate = false;
            break;
          }

          Subnet::Link link(bestReplacementMap.at(idx).cellIDInMappedSubnet);
          linkList.push_back(link);
        }

        if (readyForCreate) {
          if (currentCell.isOut() || currentCell.type == eda::gate::model::CellSymbol::OUT) {
            auto cellID = subnetBuilder.addOutput(linkList[0]);
            bestReplacementMap[currentEntryIDX].cellIDInMappedSubnet = cellID;
          } else {

            /*Subnet &techSubnet = Subnet::get(bestReplacementMap[currentEntryIDX].getLibertySubnetID());
            eda::gate::model::Array<Subnet::Entry> techCellEntries = techSubnet.getEntries();
            for (const auto &techCellEntry : techCellEntries) {
              auto techCell = techCellEntry.cell;
              if (!techCell.isIn() && !techCell.isOut()) {
                auto cellID = subnetBuilder.addCell(techCell.getTypeID(), linkList);
                bestReplacementMap[currentEntryIDX].cellIDInMappedSubnet = cellID;
                countOfCell++;
              }
            }*/
            //std::cout << Subnet::get(bestReplacementMap[currentEntryIDX].subnetID) << std::endl;
            auto cellID = subnetBuilder.addSingleOutputSubnet(bestReplacementMap[currentEntryIDX].subnetID,
                                                  linkList);
            bestReplacementMap[currentEntryIDX].cellIDInMappedSubnet = cellID.idx;
            std::cout << "add subnet out index: " << cellID.idx << std::endl;
          }
          stack.pop();
        }
      }

      for (const auto &link : bestReplacementMap[currentEntryIDX].entryIDxs) {
        if (visited.find(link) == visited.end()) {
          stack.push(link);
          visited.insert(link);
        }
      }
    }
    std::cout << "Count of New Cell = " << countOfCell << std::endl;
    return subnetBuilder.make();
  }

  float CutBasedTechMapper::getArea() const{
    return area;
  }
  float CutBasedTechMapper::getDelay() const{
    return delay;
  }
} // namespace eda::gate::tech_optimizer
