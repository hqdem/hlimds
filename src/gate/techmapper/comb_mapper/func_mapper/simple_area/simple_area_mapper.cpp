//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/techmapper/comb_mapper/func_mapper/simple_area/simple_area_mapper.h"

#include <cassert>

namespace eda::gate::techmapper {

using Subnet = model::Subnet;
using SubnetBuilder = model::SubnetBuilder;
using SubnetView = model::SubnetView;
using Type = MappingItem::Type;

void SimpleAreaMapper::map(
       const SubnetID subnetID, const SCLibrary &cellDB,
       const SDC &sdc, Mapping &mapping) {
  this->subnetID = subnetID;
  cutExtractor = new optimizer::CutExtractor(&model::Subnet::get(subnetID), 6);
  Subnet &subnet = Subnet::get(subnetID);

  model::Array<Subnet::Entry> entries = subnet.getEntries();
  for (uint64_t entryIndex = 0; entryIndex < std::size(entries);
       entryIndex++) {
    auto cell = entries[entryIndex].cell;

    if (!(cell.isAnd() || cell.isBuf())) {
      addNotAnAndToTheMap(entryIndex, cell, mapping);
    } else {
      saveBest(entryIndex, cutExtractor->getCuts(entryIndex), cellDB, mapping);
    }
    entryIndex += cell.more;
  }
}

float SimpleAreaMapper::dynamicCalculateArea(
        const EntryIndex entryIndex,
        const std::unordered_set<uint64_t> &entryIdxs,
        const SCLibrary &cellDB, Mapping &mapping) {
  float area = 0;
  std::stack<EntryIndex> stack;
  std::unordered_set<EntryIndex> visited;

  for (const auto& out : entryIdxs) {
    stack.push(out);
    visited.insert(out);
    assert(mapping.find(out) != mapping.end());
    const auto type = mapping.at(out).getType();
    if (type != Type::ZERO && type != Type::ONE && type != Type::IN) {
      area += cellDB.getCellAttrs(
                mapping.at(out).getSubnetID()).area;
    }
  }

  while (!stack.empty()) {
    EntryIndex topEntryIndex = stack.top();
    assert(mapping.find(topEntryIndex) != mapping.end());

    for (const auto &in : mapping.at(topEntryIndex).inputs) {
      if (visited.find(in) == visited.end()) {
        stack.push(in);
        visited.insert(in);
      }
    }
    const auto type = mapping.at(topEntryIndex).getType();
    if (type != Type::ZERO && type != Type::ONE && type != Type::IN) {
      area += cellDB.getCellAttrs(
                mapping.at(topEntryIndex).getSubnetID()).area;
    }
    stack.pop();
  }
  return area;
}

void SimpleAreaMapper::saveBest(
       const EntryIndex entryIndex,
       const optimizer::CutExtractor::CutsList &cutsList,
       const SCLibrary &cellDB, Mapping &mapping) {

  SubnetID bestTechCellSubnetID;
  optimizer::CutExtractor::Cut bestCut;
  float bestArea = MAXFLOAT;

  SubnetBuilder builder(subnetID); // FIXME:

  for (const auto &cut : cutsList) {
    if (cut.entryIdxs.count(entryIndex) != 1) {
      SubnetView window(builder, cut);
      const auto truthTable = window.evaluateTruthTable();

      for (const SubnetID &currentSubnetID : cellDB.getSubnetID(truthTable)) {
        float area = dynamicCalculateArea(entryIndex, cut.entryIdxs, cellDB, mapping);
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

  mapping[entryIndex].setSubnetID(bestTechCellSubnetID);
  for (const auto &in : bestCut.entryIdxs) {
    mapping[entryIndex].inputs.push_back(in);
  }
}

} // namespace eda::gate::techmapper
