//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/cut_extractor.h"

namespace eda::gate::optimizer {

uint16_t countSetBits(uint64_t x) {
  x = x - ((x >> 1) & 0x5555555555555555ull);
  x = (x & 0x3333333333333333UL) + ((x >> 2) & 0x3333333333333333ull);
  x = (((x + (x >> 4)) & 0xF0F0F0F0F0F0F0Full) * 0x101010101010101ull) >> 56;
  return static_cast<uint16_t>(x);
}

CutExtractor::CutExtractor(const Subnet *subnet, const uint16_t k):
    subnet(subnet),
    builder(nullptr),
    k(k) {
  // Cuts for subnets are computed in advance.
  const auto &entries = subnet->getEntries();
  entriesCuts.resize(entries.size());
  for (size_t i = 0; i < entries.size(); ++i) {
    findCuts(i);
    i += entries[i].cell.more;
  }
}

CutExtractor::CutExtractor(const SubnetBuilder *builder,
                           const uint16_t k,
                           const bool extractNow):
    subnet(nullptr),
    builder(builder),
    k(k) {
  // Cuts might be (re)computed on demand.
  if (extractNow) {
    entriesCuts.resize(builder->getMaxIdx() + 1);
    for (auto it = builder->begin(); it != builder->end(); it.nextCell()) {
      findCuts(*it);
    }
  }
}

void CutExtractor::recomputeCuts(const size_t entryIdx) {
  if (entriesCuts.size() <= entryIdx) {
    entriesCuts.resize(entryIdx + 1);
  }
  findCuts(entryIdx);
}

const CutExtractor::CutsList CutExtractor::getCuts(
    const size_t entryIdx) const {
  return entriesCuts[entryIdx];
}

CutExtractor::CutsEntries CutExtractor::getCutsEntries(
    const size_t entryIdx) const {
  CutsEntries cutsEntries;
  for (const auto &cut : getCuts(entryIdx)) {
    cutsEntries.push_back(cut.entryIdxs);
  }
  return cutsEntries;
}

CutExtractor::LinkList CutExtractor::getLinks(const size_t entryID) const {
  if (subnet) {
    return subnet->getLinks(entryID);
  }
  return builder->getLinks(entryID);
}

void CutExtractor::findCuts(const size_t entryIdx) {
  auto entryLinks = getLinks(entryIdx);
  RawCutsList cuts;
  cuts.push_back({ getOneElemCut(entryIdx), true });
  if (!entryLinks.size()) {
    addViableCuts(cuts, entryIdx);
    return;
  }
  uint64_t cutsCombinationsN = 1;
  std::vector<size_t> suffCutsCombinationsN(entryLinks.size());
  for (long long i = entryLinks.size() - 1; i >= 0; --i) {
    cutsCombinationsN *= entriesCuts[entryLinks[i].idx].size();
    suffCutsCombinationsN[i] = cutsCombinationsN;
  }
  for (size_t i = 0; i < cutsCombinationsN; ++i) {
    addCut(entryIdx, i, cuts, suffCutsCombinationsN);
  }
  addViableCuts(cuts, entryIdx);
}

void CutExtractor::addCut(
    const size_t entryIdx,
    uint64_t cutsCombinationIdx,
    RawCutsList &addedCuts,
    const std::vector<size_t> &suffCutsCombN) const {
  Cut newCut;
  newCut.rootEntryIdx = entryIdx;
  const auto &entryLinks = getLinks(entryIdx);

  if (countSetBits(getNewCutSign(cutsCombinationIdx,
                                 entryLinks, suffCutsCombN)) > k) {
    return;
  }

  for (size_t j = 0; j < entryLinks.size(); ++j) {
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

CutExtractor::Cut CutExtractor::getOneElemCut(const size_t entryIdx) const {
  return Cut(entryIdx, (size_t)1 << (entryIdx % 64), { entryIdx });
}

bool CutExtractor::cutNotDominated(const Cut &cut, RawCutsList &cuts) const {
  for (size_t i = 0; i < cuts.size(); ++i) {
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
    uint64_t cutsCombinationIdx,
    const LinkList &entryLinks,
    const std::vector<size_t> &suffCutsCombN) const {

  uint64_t newCutSignature = 0;

  for (size_t j = 0; j < entryLinks.size(); ++j) {
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

} // namespace eda::gate::optimizer
