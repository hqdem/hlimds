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
using Type = eda::gate::techmapper::BestReplacement::Type;

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
    const auto type = bestReplacementMap->at(out).getType();
    if (type != Type::ZERO && type != Type::ONE && type != Type::IN) {
      area += cellDB->getSubnetAttrBySubnetID(
                bestReplacementMap->at(out).getSubnetID()).area;
    }
  }

  while (!stack.empty()) {
    EntryIndex topEntryIndex = stack.top();
    assert(bestReplacementMap->find(topEntryIndex) != bestReplacementMap->end());

    for (const auto &in : bestReplacementMap->at(topEntryIndex).inputs) {
      if (visited.find(in) == visited.end()) {
        stack.push(in);
        visited.insert(in);
      }
    }
    const auto type = bestReplacementMap->at(topEntryIndex).getType();
    if (type != Type::ZERO && type != Type::ONE && type != Type::IN) {
      area += cellDB->getSubnetAttrBySubnetID(
                bestReplacementMap->at(topEntryIndex).getSubnetID()).area;
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
        float area = dynamicCalculateArea(entryIndex, cut.entryIdxs);
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

  (*bestReplacementMap)[entryIndex].setSubnetID(bestTechCellSubnetID);
  for (const auto &in : bestCut.entryIdxs) {
    (*bestReplacementMap)[entryIndex].inputs.push_back(in);
  }
}

} // namespace eda::gate::techmapper
