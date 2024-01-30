//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/techoptimizer/mapper/cut_base/simple_area/simple_area_mapper.h"
#include "gate/optimizer2/cone_builder.h"
#include "gate/model2/utils/subnet_truth_table.h"

namespace eda::gate::tech_optimizer {
void SimpleAreaMapper::findBest() {
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
      saveBest(entryIndex, cutExtractor->getCuts(entryIndex));
    }
    entryIndex += cell.more;
  }
}


float SimpleAreaMapper::calculateArea(const std::unordered_set<uint64_t> &entryIdxs){
  float area = 0;
  Subnet &subnet = Subnet::get(subnetID);
  eda::gate::model::Array<Subnet::Entry> entries = subnet.getEntries();

  std::stack<uint64_t> stack;
  std::unordered_set<uint64_t> visited;

  for (const auto& out : entryIdxs) {
    stack.push(out);
    visited.insert(out);
  }

  while (!stack.empty()) {
    EntryIndex currentEntryIDX = stack.top();
    auto currentCell = entries[currentEntryIDX].cell;
    if ((!currentCell.isIn() || currentCell.getSymbol() != model::CellSymbol::IN)
        && (!currentCell.isOut() || currentCell.getSymbol() != model::CellSymbol::OUT)) {
      area += cellDB->getSubnetAttrBySubnetID(bestReplacementMap->at(
          currentEntryIDX).subnetID).area;
    }
    stack.pop();
    for (const auto &link : currentCell.link) {
      if (visited.find(link.idx) == visited.end()) {
        stack.push(link.idx);
        visited.insert(link.idx);
      }
    }
  }
  return area;
}

void SimpleAreaMapper::saveBest(
    EntryIndex entryIndex,
    const optimizer2::CutExtractor::CutsList &cutsList) {
  eda::gate::optimizer2::ConeBuilder coneBuilder(&Subnet::get(subnetID));
  BestReplacement bestSimpleReplacement{};
  float bestArea = MAXFLOAT;
  // Iterate over all cuts to find the best replacement
  for (const auto &cut : cutsList) {
    if (cut.entryIdxs.size() != 1) {

      SubnetID coneSubnetID = coneBuilder.getCone(cut).subnetID;

      auto truthTable = eda::gate::model::evaluate(
          model::Subnet::get(coneSubnetID));

      for (const SubnetID &currentSubnetID : cellDB->getSubnetIDsByTT(truthTable)) {
        auto currentAttr = cellDB->getSubnetAttrBySubnetID(currentSubnetID);

        float area = calculateArea(cut.entryIdxs)
                     + currentAttr.area;

        if (area < bestArea) {
          bestArea = area;
          bestSimpleReplacement.subnetID = currentSubnetID;
          bestSimpleReplacement.entryIDxs = cut.entryIdxs;
        }
      }
    }
  }
  (*bestReplacementMap)[entryIndex] = bestSimpleReplacement;
}

void SimpleAreaMapper::addInputToTheMap(EntryIndex entryIndex) {
  BestReplacement bestReplacement{true};
  (*bestReplacementMap)[entryIndex] = bestReplacement;
}
void SimpleAreaMapper::addZeroToTheMap(EntryIndex entryIndex) {
  BestReplacement bestReplacement{};
  bestReplacement.isZero = true;
  (*bestReplacementMap)[entryIndex] = bestReplacement;
}
void SimpleAreaMapper::addOneToTheMap(EntryIndex entryIndex) {
  BestReplacement bestReplacement{};
  bestReplacement.isOne = true;
  (*bestReplacementMap)[entryIndex] = bestReplacement;
}
void SimpleAreaMapper::addOutToTheMap(EntryIndex entryIndex,
                                        Subnet::Cell &cell) {
  BestReplacement bestReplacement{false, true};
  bestReplacement.entryIDxs.insert(cell.link[0].idx);
  (*bestReplacementMap)[entryIndex] = bestReplacement;
}
}