//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#include <assert.h>

#include "gate/model2/utils/subnet_truth_table.h"
#include "gate/optimizer2/cone_builder.h"
#include "gate/techmapper/mapper/cut_base/simple_area/simple_area_mapper.h"

namespace eda::gate::tech_optimizer {
void SimpleAreaMapper::findBest() {
  Subnet &subnet = Subnet::get(subnetID);

  for (uint16_t i = 0; i < subnet.getInNum(); i++) {
    areaVec[i] = BestReplacementArea{0,{}};
  }

  eda::gate::model::Array<Subnet::Entry> entries = subnet.getEntries();
  for (uint64_t entryIndex = 0; entryIndex < std::size(entries);
       entryIndex++) {
    auto cell = entries[entryIndex].cell;

    if (!(cell.isAnd() || cell.isBuf())) {
      addNotAnAndToTheMap(entryIndex, cell);
    } else {
      // Save best tech cells subnet to bestReplMap
      saveBest(entryIndex, cutExtractor->getCuts(entryIndex));
    }
    entryIndex += cell.more;
  }
  areaVec.clear();
}


float SimpleAreaMapper::calculateArea(const std::unordered_set<uint64_t> &entryIdxs, EntryIndex currnetEntry){
  std::unordered_set<uint64_t> incomingEntries;
  float area = 0;
  for (const auto &in : entryIdxs) {
    area += areaVec.at(in).area;
    for (const auto &inEntry : areaVec.at(in).incomingEntries) {
      if (!incomingEntries.insert(inEntry).second) {
        area -= areaVec.at(inEntry).area;
      }
    }
  }
  std::vector<EntryIndex> inEntry;
  for (const auto &in : incomingEntries) {
    inEntry.push_back(in);
  }
  areaVec[currnetEntry] = BestReplacementArea{area, inEntry};
  return area;
}

void SimpleAreaMapper::saveBest(
    EntryIndex entryIndex,
    const optimizer2::CutExtractor::CutsList &cutsList) {
  optimizer2::ConeBuilder coneBuilder(&Subnet::get(subnetID));
  BestReplacement bestSimpleReplacement{};
  float bestArea = MAXFLOAT;

  assert(cutsList.size() > 1);

  // Iterate over all cuts to find the best replacement
  for (const auto &cut : cutsList) {
    if (cut.entryIdxs.count(entryIndex) != 1) {

      SubnetID coneSubnetID = coneBuilder.getCone(cut).subnetID;

      auto truthTable = eda::gate::model::evaluate(
          model::Subnet::get(coneSubnetID));

      for (const SubnetID &currentSubnetID : cellDB->getSubnetIDsByTT(truthTable.at(0))) {
        auto currentAttr = cellDB->getSubnetAttrBySubnetID(currentSubnetID);

        float area = calculateArea(cut.entryIdxs, entryIndex)
                     + currentAttr.area;

        if (area < bestArea) {
          bestArea = area;
          bestSimpleReplacement.subnetID = currentSubnetID;
          for (const auto &in : cut.entryIdxs) {
            bestSimpleReplacement.entryIDxs.push_back(in);
          }
        }
      }
    }
  }

  assert(bestArea != MAXFLOAT);

  areaVec[entryIndex] = {bestArea, bestSimpleReplacement.entryIDxs};
  if (bestSimpleReplacement.entryIDxs.empty()) {
    std::cout << entryIndex << std::endl;
  }
  assert(!bestSimpleReplacement.entryIDxs.empty());
  (*bestReplacementMap)[entryIndex] = bestSimpleReplacement;
}
} // namespace eda::gate::tech_optimizer