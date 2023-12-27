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

void ConeBuilder::addInput(const uint64_t origEntryIdx,
                           const uint64_t rootEntryIdx,
                           SubnetBuilder &builder,
                           EntryMap &origEntryToCone,
                           EntryMap &coneEntryToOrig) const {

  const auto &origCell = subnet->getEntries()[origEntryIdx].cell;
  const auto inputKind = (origEntryIdx == rootEntryIdx) ? SubnetBuilder::INOUT :
                                                          SubnetBuilder::INPUT;
  uint64_t coneEntryIdx = builder.addCell(origCell.getSymbol(), inputKind);
  origEntryToCone[origEntryIdx] = coneEntryIdx;
  coneEntryToOrig[coneEntryIdx] = origEntryIdx;
}

void ConeBuilder::addInsFromCut(const Cut &cut,
                                SubnetBuilder &builder,
                                EntryMap &origEntryToCone,
                                EntryMap &coneEntryToOrig) const {

  for (const auto &inEntryIdx : cut.entryIdxs) {
    addInput(inEntryIdx, cut.rootEntryIdx, builder, origEntryToCone,
             coneEntryToOrig);
  }
}

void ConeBuilder::addInsForMaxCone(const uint64_t rootEntryIdx,
                                   SubnetBuilder &builder,
                                   EntryMap &origEntryToCone,
                                   EntryMap &coneEntryToOrig) const {

  const auto entries = subnet->getEntries();
  std::unordered_map<uint64_t, char> used;
  std::vector<uint64_t> subnetEntriesStack;
  subnetEntriesStack.push_back(rootEntryIdx);
  std::size_t stackIdx = 0;
  while (stackIdx < subnetEntriesStack.size()) {
    uint64_t origEntryIdx = subnetEntriesStack[stackIdx];
    used[origEntryIdx] = true;
    stackIdx++;
    const auto &origCell = entries[origEntryIdx].cell;
    if (origCell.isIn() || origCell.isZero() || origCell.isOne()) {
      addInput(origEntryIdx, rootEntryIdx, builder, origEntryToCone,
               coneEntryToOrig);
      continue;
    }
    for (const auto &newEntry : subnet->getLinks(origEntryIdx)) {
      uint64_t newEntryIdx = newEntry.idx;
      if (used.find(newEntryIdx) != used.end()) {
        continue;
      }
      subnetEntriesStack.push_back(newEntryIdx);
    }
  }
}

ConeBuilder::Cone ConeBuilder::getCone(const Cut &cut) const {
  SubnetBuilder builder;
  EntryMap origEntryToCone;
  EntryMap coneEntryToOrig;

  addInsFromCut(cut, builder, origEntryToCone, coneEntryToOrig);
  return getCone(cut.rootEntryIdx, builder, origEntryToCone, coneEntryToOrig);
}

ConeBuilder::Cone ConeBuilder::getMaxCone(const uint64_t rootEntryIdx) const {
  SubnetBuilder builder;
  EntryMap origEntryToCone;
  EntryMap coneEntryToOrig;

  addInsForMaxCone(rootEntryIdx, builder, origEntryToCone, coneEntryToOrig);
  return getCone(rootEntryIdx, builder, origEntryToCone, coneEntryToOrig);
}

ConeBuilder::Cone ConeBuilder::getCone(const uint64_t rootEntryIdx,
                                       SubnetBuilder &builder,
                                       EntryMap &origEntryToCone,
                                       EntryMap &coneEntryToOrig) const {

  const auto &entries = subnet->getEntries();
  std::stack<uint64_t> subnetEntriesStack;
  subnetEntriesStack.push(rootEntryIdx);

  while (!subnetEntriesStack.empty()) {
    uint64_t curEntryIdx = subnetEntriesStack.top();
    if (origEntryToCone.find(curEntryIdx) != origEntryToCone.end()) {
      subnetEntriesStack.pop();
      continue;
    }
    auto curCell = entries[curEntryIdx].cell;

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
