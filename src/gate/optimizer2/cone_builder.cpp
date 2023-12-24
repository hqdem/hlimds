//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer2/cone_builder.h"

namespace eda::gate::optimizer2 {

ConeBuilder::ConeBuilder(const Subnet *subnet): subnet(subnet) {};

ConeBuilder::Cone ConeBuilder::getCone(const Cut &cut) const {
  const auto isEntryFromCut = [&] (uint64_t curEntryIdx) {
    return cut.entryIdxs.find(curEntryIdx) != cut.entryIdxs.end();
  };
  return getCone(cut.rootEntryIdx, isEntryFromCut);
}

ConeBuilder::Cone ConeBuilder::getMaxCone(const uint64_t rootEntryIdx) const {
  const auto isEntryInput = [&] (uint64_t curEntryIdx) {
    const auto curCell = subnet->getEntries()[curEntryIdx].cell;
    return curCell.isIn() || curCell.isOne() || curCell.isZero();
  };
  return getCone(rootEntryIdx, isEntryInput);
}

auto ConeBuilder::getCone(const uint64_t rootEntryIdx,
                          const EntryCheckFunc &isInEntry) const ->
  ConeBuilder::Cone {

  SubnetBuilder builder;
  EntryMap origEntryToCone;
  EntryMap coneEntryToOrig;
  std::stack<uint64_t> subnetEntriesStack;
  subnetEntriesStack.push(rootEntryIdx);
  const auto &entries = subnet->getEntries();

  while (!subnetEntriesStack.empty()) {
    uint64_t curEntryIdx = subnetEntriesStack.top();
    if (origEntryToCone.find(curEntryIdx) != origEntryToCone.end()) {
      subnetEntriesStack.pop();
      continue;
    }
    auto curCell = entries[curEntryIdx].cell;
    if (isInEntry(curEntryIdx)) {
      subnetEntriesStack.pop();
      uint64_t coneEntryIdx;
      if (curEntryIdx == rootEntryIdx) {
        coneEntryIdx = builder.addCell(curCell.getSymbol(),
                                       SubnetBuilder::INOUT);
      } else {
        coneEntryIdx = builder.addCell(curCell.getSymbol(),
                                       SubnetBuilder::INPUT);
      }
      origEntryToCone[curEntryIdx] = coneEntryIdx;
      coneEntryToOrig[coneEntryIdx] = curEntryIdx;
      continue;
    }

    LinkList links;
    bool allInputsVisited = true;
    for (const auto &newEntry : subnet->getLinks(curEntryIdx)) {
      const auto newEntryIdx = newEntry.idx;
      if (origEntryToCone.find(newEntryIdx) == origEntryToCone.end()) {
        subnetEntriesStack.push(newEntryIdx);
        allInputsVisited = false;
      }
      if (allInputsVisited) {
        links.push_back(origEntryToCone[newEntryIdx]);
      }
    }

    if (!allInputsVisited) {
      continue;
    }
    subnetEntriesStack.pop();
    uint64_t coneEntryIdx;
    if (curEntryIdx == rootEntryIdx) {
      coneEntryIdx = builder.addCell(curCell.getSymbol(), links,
                                     SubnetBuilder::OUTPUT);
    } else {
      coneEntryIdx = builder.addCell(curCell.getSymbol(), links);
    }
    origEntryToCone[curEntryIdx] = coneEntryIdx;
    coneEntryToOrig[coneEntryIdx] = curEntryIdx;
  }
  return Cone(builder.make(), coneEntryToOrig);
}

} // namespace eda::gate::optimizer2
