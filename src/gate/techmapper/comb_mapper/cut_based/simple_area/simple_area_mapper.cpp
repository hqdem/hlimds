//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/utils/subnet_truth_table.h"
#include "gate/optimizer/cone_builder.h"
#include "gate/techmapper/comb_mapper/cut_based/simple_area/simple_area_mapper.h"

#include <cassert>

using Subnet = eda::gate::model::Subnet;

namespace eda::gate::techmapper {

void SimpleAreaMapper::findBest() {
  Subnet &subnet = Subnet::get(subnetID);

  for (uint16_t i = 0; i < subnet.getInNum(); i++) {
    areaVec[i] = BestReplacementArea{0,{}};
  }

  model::Array<Subnet::Entry> entries = subnet.getEntries();
  for (uint64_t entryIndex = 0; entryIndex < std::size(entries);
       entryIndex++) {
    auto cell = entries[entryIndex].cell;

    if (!(cell.isAnd() || cell.isBuf())) {
      addNotAnAndToTheMap(entryIndex, cell);
    } else {
      saveBest(entryIndex, cutExtractor->getCuts(entryIndex));
    }
    entryIndex += cell.more;
  }
  areaVec.clear();
}

float SimpleAreaMapper::calculateArea(
                          const std::unordered_set<uint64_t> &entryIdxs,
                          EntryIndex currnetEntry) {
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

void SimpleAreaMapper::saveBest( EntryIndex entryIndex,
    const optimizer::CutExtractor::CutsList &cutsList) {

  SubnetID bestTechCellSubnetID;
  optimizer::CutExtractor::Cut bestCut;
  float bestArea = MAXFLOAT;

  optimizer::ConeBuilder coneBuilder(&Subnet::get(subnetID));

  for (const auto &cut : cutsList) {
    if (cut.entryIdxs.count(entryIndex) != 1) {
      auto truthTable = eda::gate::model::evaluate(
          Subnet::get(coneBuilder.getCone(cut).subnetID)).at(0);

      for (const SubnetID &currentSubnetID : cellDB->getSubnetIDsByTT(truthTable)) {
        float area = calculateArea(cut.entryIdxs, entryIndex) +
            cellDB->getSubnetAttrBySubnetID(currentSubnetID).area;

        if (area < bestArea) {
          bestArea = area;
          bestTechCellSubnetID = currentSubnetID;
          bestCut = cut;
        }
      }
    }
  }

  assert(bestArea != MAXFLOAT);
  assert(!bestCut.entryIdxs.empty());

  (*bestReplacementMap)[entryIndex].subnetID = bestTechCellSubnetID;
  for (const auto &in : bestCut.entryIdxs) {
    (*bestReplacementMap)[entryIndex].entryIDxs.push_back(in);
  }

  areaVec[entryIndex] = {bestArea, (*bestReplacementMap)[entryIndex].entryIDxs};
}
} // namespace eda::gate::techmapper
