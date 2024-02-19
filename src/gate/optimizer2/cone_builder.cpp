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

void ConeBuilder::addInput(const size_t origEntryIdx,
                           const size_t rootEntryIdx,
                           SubnetBuilder &builder,
                           EntryMap &origEntryToCone,
                           EntryMap &coneEntryToOrig) const {

  size_t coneEntryIdx = builder.addInput().idx;
  if (origEntryIdx == rootEntryIdx) {
    builder.addOutput(Link(coneEntryIdx));
  }
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

void ConeBuilder::addInsForMaxCone(const size_t rootEntryIdx,
                                   SubnetBuilder &builder,
                                   EntryMap &origEntryToCone,
                                   EntryMap &coneEntryToOrig) const {

  const auto entries = subnet->getEntries();
  std::unordered_map<size_t, char> used;
  std::vector<size_t> subnetEntriesStack;
  subnetEntriesStack.push_back(rootEntryIdx);
  size_t stackIdx = 0;
  while (stackIdx < subnetEntriesStack.size()) {
    size_t origEntryIdx = subnetEntriesStack[stackIdx];
    used[origEntryIdx] = true;
    stackIdx++;
    const auto &origCell = entries[origEntryIdx].cell;
    if (origCell.isIn() || origCell.isZero() || origCell.isOne()) {
      addInput(origEntryIdx, rootEntryIdx, builder, origEntryToCone,
               coneEntryToOrig);
      continue;
    }
    for (const auto &newLink : subnet->getLinks(origEntryIdx)) {
      size_t newLinkIdx = newLink.idx;
      if (used.find(newLinkIdx) != used.end()) {
        continue;
      }
      subnetEntriesStack.push_back(newLinkIdx);
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

ConeBuilder::Cone ConeBuilder::getMaxCone(const size_t rootEntryIdx) const {
  SubnetBuilder builder;
  EntryMap origEntryToCone;
  EntryMap coneEntryToOrig;

  addInsForMaxCone(rootEntryIdx, builder, origEntryToCone, coneEntryToOrig);
  return getCone(rootEntryIdx, builder, origEntryToCone, coneEntryToOrig);
}

ConeBuilder::Cone ConeBuilder::getCone(const size_t rootEntryIdx,
                                       SubnetBuilder &builder,
                                       EntryMap &origEntryToCone,
                                       EntryMap &coneEntryToOrig) const {

  const auto &entries = subnet->getEntries();
  std::stack<size_t> subnetEntriesStack;
  subnetEntriesStack.push(rootEntryIdx);

  while (!subnetEntriesStack.empty()) {
    size_t curEntryIdx = subnetEntriesStack.top();
    if (origEntryToCone.find(curEntryIdx) != origEntryToCone.end()) {
      subnetEntriesStack.pop();
      continue;
    }
    auto curCell = entries[curEntryIdx].cell;

    LinkList links;
    bool allInputsVisited = true;
    for (const auto &newLink : subnet->getLinks(curEntryIdx)) {
      const auto newLinkIdx = newLink.idx;
      if (origEntryToCone.find(newLinkIdx) == origEntryToCone.end()) {
        subnetEntriesStack.push(newLinkIdx);
        allInputsVisited = false;
      }
      if (allInputsVisited) {
        links.push_back(Link(origEntryToCone[newLinkIdx], newLink.out,
                             newLink.inv));
      }
    }

    if (!allInputsVisited) {
      continue;
    }
    subnetEntriesStack.pop();
    size_t coneEntryIdx;
    coneEntryIdx = builder.addCell(curCell.getSymbol(), links).idx;
    origEntryToCone[curEntryIdx] = coneEntryIdx;
    coneEntryToOrig[coneEntryIdx] = curEntryIdx;
    if (curEntryIdx == rootEntryIdx) {
      builder.addOutput(Link(coneEntryIdx));
    }
  }
  return Cone(builder.make(), coneEntryToOrig);
}

} // namespace eda::gate::optimizer2
