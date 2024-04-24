//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer2/cut_extractor.h"

namespace eda::gate::optimizer2 {

unsigned short countSetBits(unsigned long long x) {
  x = x - ((x >> 1) & 0x5555555555555555UL);
  x = (x & 0x3333333333333333UL) + ((x >> 2) & 0x3333333333333333UL);
  return (unsigned short)((((x + (x >> 4)) & 0xF0F0F0F0F0F0F0FUL) *
                           0x101010101010101UL) >> 56);
}

CutExtractor::CutExtractor(const Subnet *subnet, const unsigned k) :
    subnet(subnet),
    saveSubnetEntries(new model::Array<Entry>(subnet->getEntries())),
    builder(nullptr), k(k) {

  entriesCuts.resize(saveSubnetEntries->size(), {});
  for (std::size_t i = 0; i < saveSubnetEntries->size(); ++i) {
    findCuts(i);
    i += (*saveSubnetEntries)[i].cell.more;
  }
};

CutExtractor::CutExtractor(SubnetBuilder *builder, const unsigned k) :
    subnet(nullptr), saveSubnetEntries(nullptr), builder(builder), k(k) {};

void CutExtractor::recomputeCuts(size_t entryIdx) {
  if (entriesCuts.size() <= entryIdx) {
    entriesCuts.resize(entryIdx + 1);
  }
  findCuts(entryIdx);
}

const CutExtractor::CutsList CutExtractor::getCuts(size_t entryIdx) const {
  return entriesCuts[entryIdx];
}

auto CutExtractor::getCutsEntries(size_t entryIdx) const ->
  CutExtractor::CutsEntries {

  CutsEntries cutsEntries;
  for (const auto &cut : getCuts(entryIdx)) {
    cutsEntries.push_back(cut.entryIdxs);
  }
  return cutsEntries;
}

CutExtractor::LinkList CutExtractor::getLinks(size_t entryID) const {
  if (subnet) {
    //TODO: fixme
    const auto &cell = (*saveSubnetEntries)[entryID].cell;
    LinkList links(cell.arity);
    size_t j = 0;
    for (; j < cell.arity && j < Subnet::Cell::InPlaceLinks; ++j) {
      links[j] = cell.link[j];
    }
    for (; j < cell.arity; ++j) {
      const auto k = subnet->getLinkIndices(entryID, j);
      links[j] = (*saveSubnetEntries)[k.first].link[k.second];
    }
    return links;
  }
  return builder->getLinks(entryID);
}

void CutExtractor::findCuts(const size_t entryIdx) {
  const auto &entryLinks = getLinks(entryIdx);
  RawCutsList cuts;
  cuts.push_back({ getOneElemCut(entryIdx), true });
  if (!entryLinks.size()) {
    addViableCuts(cuts, entryIdx);
    return;
  }
  unsigned long long cutsCombinationsN = 1;
  std::vector<std::size_t> suffCutsCombinationsN(entryLinks.size());
  for (long long i = entryLinks.size() - 1; i >= 0; --i) {
    cutsCombinationsN *= entriesCuts[entryLinks[i].idx].size();
    suffCutsCombinationsN[i] = cutsCombinationsN;
  }
  for (std::size_t i = 0; i < cutsCombinationsN; ++i) {
    addCut(entryIdx, i, cuts, suffCutsCombinationsN);
  }
  addViableCuts(cuts, entryIdx);
}

void CutExtractor::addCut(
    const size_t entryIdx,
    unsigned long long cutsCombinationIdx,
    RawCutsList &addedCuts,
    const std::vector<std::size_t> &suffCutsCombN) const {

  Cut newCut;
  newCut.rootEntryIdx = entryIdx;
  const auto &entryLinks = getLinks(entryIdx);

  if (countSetBits(getNewCutSign(cutsCombinationIdx,
                                 entryLinks, suffCutsCombN)) > k) {
    return;
  }

  for (std::size_t j = 0; j < entryLinks.size(); ++j) {
    size_t inEntryIdx = entryLinks[j].idx;
    size_t inEntryCutIdx = cutsCombinationIdx;
    if (j + 1 != entryLinks.size()) {
      inEntryCutIdx = cutsCombinationIdx / suffCutsCombN[j + 1];
      cutsCombinationIdx %= suffCutsCombN[j + 1];
    }
    const Cut &cutToUnite = entriesCuts[inEntryIdx][inEntryCutIdx];
    newCut.uniteCut(cutToUnite);
    if (newCut.entryIdxs.size() > k) {
      return;
    }
  }
  if (cutNotDominated(newCut, addedCuts)) {
    addedCuts.push_back({ newCut, true });
  }
}

auto CutExtractor::getOneElemCut(const size_t entryIdx) const ->
  CutExtractor::Cut {

  return Cut(entryIdx, (size_t)1 << (entryIdx % 64), { entryIdx });
}

bool CutExtractor::cutNotDominated(const Cut &cut, RawCutsList &cuts) const {
  for (std::size_t i = 0; i < cuts.size(); ++i) {
    if (cutDominates(cuts[i].first, cut)) {
      return false;
    }
    if (!cuts[i].second) {
      continue;
    }
    if (cutDominates(cut, cuts[i].first)) {
      cuts[i].second = false;
    }
  }
  return true;
}

void CutExtractor::addViableCuts(
    const RawCutsList &cuts,
    const size_t entryIdx) {

  entriesCuts[entryIdx].clear();
  for (const auto &cut : cuts) {
    if (cut.second) {
      entriesCuts[entryIdx].push_back(cut.first);
    }
  }
}

bool CutExtractor::cutDominates(const Cut &cut1, const Cut &cut2) const {
  if (cut1.entryIdxs.size() >= cut2.entryIdxs.size()) {
    return false;
  }
  if ((cut1.signature | cut2.signature) != cut2.signature) {
    return false;
  }
  for (const auto &cut1Entry : cut1.entryIdxs) {
    if (cut2.entryIdxs.find(cut1Entry) == cut2.entryIdxs.end()) {
      return false;
    }
  }
  return true;
}

uint64_t CutExtractor::getNewCutSign(
    unsigned long long cutsCombinationIdx,
    const LinkList &entryLinks,
    const std::vector<std::size_t> &suffCutsCombN) const {

  uint64_t newCutSignature = 0;

  for (std::size_t j = 0; j < entryLinks.size(); ++j) {
    size_t inEntryIdx = entryLinks[j].idx;
    size_t inEntryCutIdx = cutsCombinationIdx;
    if (j + 1 != entryLinks.size()) {
      inEntryCutIdx = cutsCombinationIdx / suffCutsCombN[j + 1];
      cutsCombinationIdx %= suffCutsCombN[j + 1];
    }
    const Cut &cutToUnite = entriesCuts[inEntryIdx][inEntryCutIdx];
    newCutSignature |= cutToUnite.signature;
  }
  return newCutSignature;
}

} // namespace eda::gate::optimizer2
