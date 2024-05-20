//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include <assert.h>

#include "gate/model/utils/subnet_truth_table.h"
#include "gate/optimizer/cone_builder.h"
#include "gate/techmapper/comb_mapper/cut_based/simple_area/simple_area_mapper.h"

using Subnet = eda::gate::model::Subnet;

namespace eda::gate::techmapper {

void SimpleAreaMapper::findBest() {
  Subnet &subnet = Subnet::get(subnetID);

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
}

float SimpleAreaMapper::dynamicCalculateArea(EntryIndex entryIndex,
                                             const std::unordered_set<uint64_t> &entryIdxs) {
  float area = 0;
  std::stack<EntryIndex> stack;
  std::unordered_set<EntryIndex> visited;

  for (const auto& out : entryIdxs) {
    stack.push(out);
    visited.insert(out);
    assert(bestReplacementMap->find(out) != bestReplacementMap->end());
    if (!bestReplacementMap->at(out).isZero &&
        !bestReplacementMap->at(out).isOne &&
        !bestReplacementMap->at(out).isIN) {
      area += cellDB->getSubnetAttrBySubnetID(bestReplacementMap->at(out).subnetID).area;
    }
  }

  while (!stack.empty()) {
    EntryIndex currentEntryIDX = stack.top();
    assert(bestReplacementMap->find(currentEntryIDX) != bestReplacementMap->end());

    for (const auto &in : bestReplacementMap->at(currentEntryIDX).entryIDxs) {
      if (visited.find(in) == visited.end()) {
        stack.push(in);
        visited.insert(in);
      }
    }
    if (!bestReplacementMap->at(currentEntryIDX).isZero &&
        !bestReplacementMap->at(currentEntryIDX).isOne &&
        !bestReplacementMap->at(currentEntryIDX).isIN) {
      area += cellDB->getSubnetAttrBySubnetID(bestReplacementMap->at(currentEntryIDX).subnetID).area;
    }
    stack.pop();
  }
  return area;
}

void SimpleAreaMapper::saveBest(EntryIndex entryIndex,
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
        float area = dynamicCalculateArea(entryIndex,cut.entryIdxs);
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
}
} // namespace eda::gate::techmapper