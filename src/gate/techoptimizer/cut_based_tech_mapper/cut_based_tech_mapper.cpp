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
  std::vector<EntryIndex> inID;

  CutBasedTechMapper::CutBasedTechMapper(CellDB *cellDB) {
    this->cellDB = cellDB;
  }

  void CutBasedTechMapper::setStrategy(Strategy *strategy,
      std::map<uint64_t, BestReplacement> *bestReplacementMap) {
    this->strategy = strategy;
    this->bestReplacementMap = bestReplacementMap;
  }

  SubnetID CutBasedTechMapper::techMap(SubnetID subnetID) {

    auto startAIG = std::chrono::high_resolution_clock::now();
    SubnetID aigSubnet = aigMap(subnetID);
    auto endAIG = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> AIGTime = endAIG - startAIG;
    std::cout << "Функция AIG выполнялась " << AIGTime.count() << " секунд.\n";

    auto startFindBestRepl = std::chrono::high_resolution_clock::now();
    replacementSearch(aigSubnet);
    auto endFindBestRepl = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> CutFindBestReplTime = endFindBestRepl - startFindBestRepl;
    std::cout << "Функция FindBestRepl выполнялась " << CutFindBestReplTime.count() << " секунд.\n";

    auto startMapped = std::chrono::high_resolution_clock::now();
    const SubnetID mappedSubnet = buildSubnet(aigSubnet);
    auto endMapped = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> CutMappedTime = endMapped - startMapped;
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

SubnetID CutBasedTechMapper::aigMap(SubnetID subnetID) {
  transformer::AigMapper mapper;
  const auto transformedSub  = mapper.transform(subnetID);
  return transformedSub;
}

  void CutBasedTechMapper::replacementSearch(
      SubnetID subnetID) {
    CutExtractor cutExtractor(&model::Subnet::get(subnetID), MAX_CUT_SIZE);

    if (strategy->multiPass) {
      strategy->findBest(subnetID, cutExtractor, *cellDB, *bestReplacementMap);
    } else {
      singlePassSearch(subnetID, cutExtractor);
    }
  }

  SubnetID CutBasedTechMapper::buildSubnet(SubnetID subnetID) {
    Subnet &subnet = Subnet::get(subnetID);
    auto entries = subnet.getEntries();
    
    model::SubnetBuilder subnetBuilder;
    addInputCells(subnetBuilder);

    std::stack<EntryIndex> stack;
    std::unordered_set<EntryIndex> visited;

    for (const auto& out : outID) {
      stack.push(out);
      visited.insert(out);
    }

    while (!stack.empty()) {
      EntryIndex currentEntryIDX = stack.top();
      auto currentCell = entries[currentEntryIDX].cell;
      processNode(currentEntryIDX,currentCell,subnetBuilder,stack);
      processLinks(currentEntryIDX, stack, visited);
    }

    addOutputCells(subnetBuilder);
    return subnetBuilder.make();
  }

  float CutBasedTechMapper::getArea() const{
    return area;
  }
  float CutBasedTechMapper::getDelay() const{
    return delay;
  }

  void CutBasedTechMapper::singlePassSearch(SubnetID subnetID,
                                            CutExtractor &cutExtractor) {
    Subnet &subnet = Subnet::get(subnetID);

    eda::gate::model::Array<Subnet::Entry> entries = subnet.getEntries();
    for (uint64_t entryIndex = 0; entryIndex < std::size(entries);
         entryIndex++) {
      auto cell = entries[entryIndex].cell;

      if (cell.isIn()) {
        addInputToTheMap(entryIndex);
      } else if (cell.isOne()) {
        addOneToTheMap(entryIndex);
      } else if (cell.isZero()) {
        addZeroToTheMap(entryIndex);
      } else if (cell.isOut()) {
        addOutToTheMap(entryIndex, cell);
      } else {
        // Save best tech cells subnet to bestReplMap
        strategy->findBest(entryIndex, cutExtractor.getCuts(entryIndex),
                           *bestReplacementMap, *cellDB, subnetID);
      }
      entryIndex += cell.more;
    }
  }

void CutBasedTechMapper::addInputToTheMap(EntryIndex entryIndex) {
  BestReplacement bestReplacement{true};
  (*bestReplacementMap)[entryIndex] = bestReplacement;
  inID.push_back(entryIndex);
}
void CutBasedTechMapper::addZeroToTheMap(EntryIndex entryIndex) {
  BestReplacement bestReplacement{};
  bestReplacement.isZero = true;
  (*bestReplacementMap)[entryIndex] = bestReplacement;
}
void CutBasedTechMapper::addOneToTheMap(EntryIndex entryIndex) {
  BestReplacement bestReplacement{};
  bestReplacement.isOne = true;
  (*bestReplacementMap)[entryIndex] = bestReplacement;
}
void CutBasedTechMapper::addOutToTheMap(EntryIndex entryIndex,
                                        Subnet::Cell &cell) {
  outID.push_back(entryIndex);

  BestReplacement bestReplacement{false, true};
  bestReplacement.entryIDxs.insert(cell.link[0].idx);
  (*bestReplacementMap)[entryIndex] = bestReplacement;
}

void CutBasedTechMapper::addInputCells(model::SubnetBuilder &subnetBuilder) {
  for (const auto idx : inID) {
    auto cellID = subnetBuilder.addInput();
    (*bestReplacementMap)[idx].cellIDInMappedSubnet = cellID.idx;
  }
}

void CutBasedTechMapper::addOutputCells(model::SubnetBuilder &subnetBuilder) {
  for (const auto idx : outID) {
    auto input = bestReplacementMap->at(idx).entryIDxs;
    Subnet::Link link(bestReplacementMap->at(*(input.begin())).cellIDInMappedSubnet);
    auto cellID = subnetBuilder.addOutput(link);
    (*bestReplacementMap)[idx].cellIDInMappedSubnet = cellID.idx;
  }
}

Subnet::LinkList CutBasedTechMapper::createLinkList(EntryIndex currentEntryIDX) {
  Subnet::LinkList linkList;
  for (const auto &idx : bestReplacementMap->at(
      currentEntryIDX).entryIDxs) {
    if (bestReplacementMap->at(idx).cellIDInMappedSubnet == ULLONG_MAX) {
      return {}; // Return empty list to indicate not ready
    }
    Subnet::Link link(bestReplacementMap->at(idx).cellIDInMappedSubnet);
    linkList.push_back(link);
  }
  return linkList;
}

void CutBasedTechMapper::processNode(EntryIndex currentEntryIDX,
                                     model::Subnet::Cell &currentCell,
                                     model::SubnetBuilder &subnetBuilder,
                                     std::stack<EntryIndex> &stack) {
  if (currentCell.isIn()) {
    stack.pop();
    return;
  }
  if (currentCell.isZero() || currentCell.isOne()) {
    auto symbol = currentCell.isZero() ? model::CellSymbol::ZERO : model::CellSymbol::ONE;
    auto cellID = subnetBuilder.addCell(symbol);
    (*bestReplacementMap)[currentEntryIDX].cellIDInMappedSubnet = cellID.idx;
    stack.pop();
    return;
  }
  Subnet::LinkList linkList = createLinkList(currentEntryIDX);
  if (!linkList.empty()) {
    if (!currentCell.isOut()) {
      auto cellID = subnetBuilder.addSingleOutputSubnet(bestReplacementMap->at(currentEntryIDX).subnetID, linkList);
      (*bestReplacementMap)[currentEntryIDX].cellIDInMappedSubnet = cellID.idx;
    }
    stack.pop();
  }
}
void CutBasedTechMapper::processLinks(EntryIndex currentEntryIDX,
                                      std::stack<EntryIndex> &stack,
                                      std::unordered_set<EntryIndex> &visited) {
  for (const auto& link : bestReplacementMap->at(currentEntryIDX).entryIDxs) {
    if (visited.insert(link).second) {
      stack.push(link);
    }
  }
}
} // namespace eda::gate::tech_optimizer
