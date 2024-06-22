//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/cone_builder.h"

namespace eda::gate::optimizer {

ConeBuilder::ConeBuilder(const Subnet *subnet):
    subnet(subnet), builder(nullptr) {};

ConeBuilder::ConeBuilder(const SubnetBuilder *builder):
    subnet(nullptr), builder(builder) {};

void ConeBuilder::addInput(const size_t origEntryIdx,
                           const size_t rootEntryIdx,
                           SubnetBuilder &builder,
                           EntryMap &origEntryToCone,
                           EntryVec &coneEntryToOrig) const {

  size_t coneEntryIdx = builder.addInput().idx;
  if (origEntryIdx == rootEntryIdx) {
    origEntryToCone[origEntryIdx] = coneEntryIdx;
    if (coneEntryToOrig.size() <= coneEntryIdx) {
      coneEntryToOrig.resize(coneEntryIdx + 1);
    }
    coneEntryToOrig[coneEntryIdx] = origEntryIdx;
    coneEntryIdx = builder.addOutput(Link(coneEntryIdx)).idx;
  }
  origEntryToCone[origEntryIdx] = coneEntryIdx;
  if (coneEntryToOrig.size() <= coneEntryIdx) {
    coneEntryToOrig.resize(coneEntryIdx + 1);
  }
  coneEntryToOrig[coneEntryIdx] = origEntryIdx;
}

const ConeBuilder::Entry ConeBuilder::getEntry(const size_t entryID) const {
  if (subnet) {
    const auto &entries = subnet->getEntries();
    return entries[entryID];
  }
  return builder->getEntry(entryID);
}

const ConeBuilder::LinkList ConeBuilder::getLinks(const size_t entryID) const {
  return subnet ? subnet->getLinks(entryID) : builder->getLinks(entryID);
}

void ConeBuilder::addInsForMaxCone(const size_t rootEntryIdx,
                                   SubnetBuilder &builder,
                                   EntryMap &origEntryToCone,
                                   EntryVec &coneEntryToOrig) const {

  std::unordered_map<size_t, char> used;
  std::vector<size_t> subnetEntriesStack;
  subnetEntriesStack.push_back(rootEntryIdx);
  used[rootEntryIdx] = true;
  size_t stackIdx = 0;
  while (stackIdx < subnetEntriesStack.size()) {
    size_t origEntryIdx = subnetEntriesStack[stackIdx];
    stackIdx++;
    const auto &origCell = getEntry(origEntryIdx).cell;
    if (origCell.isIn() || origCell.isZero() || origCell.isOne()) {
      addInput(origEntryIdx, rootEntryIdx, builder, origEntryToCone,
               coneEntryToOrig);
      continue;
    }
    for (const auto &newLink : getLinks(origEntryIdx)) {
      size_t newLinkIdx = newLink.idx;
      if (used.find(newLinkIdx) == used.end()) {
        subnetEntriesStack.push_back(newLinkIdx);
        used[newLinkIdx] = true;
      }
    }
  }
}

ConeBuilder::Cone ConeBuilder::getCone(const Cut &cut) const {
  return getCone(cut.rootEntryIdx, cut.entryIdxs);
}

ConeBuilder::Cone ConeBuilder::getMaxCone(const size_t rootEntryIdx) const {
  SubnetBuilder builder;
  EntryMap origEntryToCone;
  EntryVec coneEntryToOrig;

  addInsForMaxCone(rootEntryIdx, builder, origEntryToCone, coneEntryToOrig);
  return getCone(rootEntryIdx, builder, origEntryToCone, coneEntryToOrig);
}

ConeBuilder::Cone ConeBuilder::getCone(const size_t rootEntryIdx,
                                       SubnetBuilder &builder,
                                       EntryMap &origEntryToCone,
                                       EntryVec &coneEntryToOrig) const {

  std::stack<size_t> subnetEntriesStack;
  subnetEntriesStack.push(rootEntryIdx);

  while (!subnetEntriesStack.empty()) {
    size_t curEntryIdx = subnetEntriesStack.top();
    if (origEntryToCone.find(curEntryIdx) != origEntryToCone.end()) {
      subnetEntriesStack.pop();
      continue;
    }
    auto curCell = getEntry(curEntryIdx).cell;

    LinkList links;
    bool allInputsVisited = true;
    for (const auto &newLink : getLinks(curEntryIdx)) {
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
    size_t coneEntryIdx = builder.addCell(curCell.getSymbol(), links).idx;
    if (curEntryIdx == rootEntryIdx) {
      coneEntryIdx = builder.addOutput(Link(coneEntryIdx)).idx;
    }
    origEntryToCone[curEntryIdx] = coneEntryIdx;
    if (coneEntryToOrig.size() <= coneEntryIdx) {
      coneEntryToOrig.resize(coneEntryIdx + 1);
    }
    coneEntryToOrig[coneEntryIdx] = curEntryIdx;
  }

  const auto resSubnetID = builder.make(coneEntryToOrig);
  return Cone(resSubnetID, coneEntryToOrig);
}

} // namespace eda::gate::optimizer
