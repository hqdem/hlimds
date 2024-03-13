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

CutExtractor::CutExtractor(const Subnet *subnet,
                           const unsigned int k) :
    subnet(subnet), builder(nullptr), k(k) {

  const auto &entries = subnet->getEntries();
  entriesCuts.resize(entries.size(), {});
  for (std::size_t i = 0; i < entries.size(); ++i) {
    entriesCuts[i] = findCuts(i, entries[i].cell.arity);
    i += entries[i].cell.more;
  }
};

CutExtractor::CutExtractor(SubnetBuilder *builder, const unsigned k) :
    subnet(nullptr), builder(builder), k(k) {};

void CutExtractor::recomputeCuts(size_t entryIdx) {
  uint64_t cellArity;
  if (builder) {
    cellArity = builder->getEntry(entryIdx).cell.arity;
  } else {
    cellArity = subnet->getEntries()[entryIdx].cell.arity;
  }
  if (entriesCuts.size() <= entryIdx) {
    entriesCuts.resize(entryIdx + 1);
  }
  const auto &cuts = findCuts(entryIdx, cellArity);
  entriesCuts[entryIdx] = cuts;
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
    return subnet->getLinks(entryID);
  }
  return builder->getLinks(entryID);
}

CutExtractor::CutsList CutExtractor::findCuts(const size_t entryIdx,
                                              const uint64_t cellArity) const {

  RawCutsList cuts;
  cuts.push_back({ getOneElemCut(entryIdx), true });
  if (!cellArity) {
    return getViable(cuts);
  }
  unsigned long long cutsCombinationsN = 1;
  std::vector<std::size_t> suffCutsCombinationsN(cellArity);
  for (long long i = cellArity - 1; i >= 0; --i) {
    cutsCombinationsN *= entriesCuts[getLinks(entryIdx)[i].idx].size();
    suffCutsCombinationsN[i] = cutsCombinationsN;
  }
  for (std::size_t i = 0; i < cutsCombinationsN; ++i) {
    addCut(entryIdx, cellArity, i, cuts, suffCutsCombinationsN);
  }
  return getViable(cuts);
}

void CutExtractor::addCut(const size_t entryIdx,
                          const uint64_t cellArity,
                          unsigned long long cutsCombinationIdx,
                          RawCutsList &addedCuts,
                          const std::vector<std::size_t> &suffCutsCombN) const {

  bool newCutTooLarge = false;
  Cut newCut;
  newCut.rootEntryIdx = entryIdx;
  for (std::size_t j = 0; j < cellArity; ++j) {
    size_t inEntryIdx = getLinks(entryIdx)[j].idx;
    size_t inEntryCutIdx = cutsCombinationIdx;
    if (j + 1 != cellArity) {
      inEntryCutIdx = cutsCombinationIdx / suffCutsCombN[j + 1];
      cutsCombinationIdx %= suffCutsCombN[j + 1];
    }
    const Cut &cutToUnite = entriesCuts[inEntryIdx][inEntryCutIdx];

    if (newCut.entryIdxs.size() > k ||
        countSetBits(newCut.signature | cutToUnite.signature) > k) {

      newCutTooLarge = true;
      break;
    }
    newCut.uniteCut(cutToUnite);
  }
  if (!newCutTooLarge && checkViable(newCut, addedCuts)) {
    addedCuts.push_back({ newCut, true });
  }
}

auto CutExtractor::getOneElemCut(const size_t entryIdx) const ->
  CutExtractor::Cut {

  return Cut(entryIdx, (size_t)1 << (entryIdx % 64), { entryIdx });
}

bool CutExtractor::checkViable(const Cut &cut,
                               RawCutsList &cuts) const {

  if (cut.entryIdxs.size() > k) {
    return false;
  }
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

CutExtractor::CutsList CutExtractor::getViable(const RawCutsList &cuts) const {

  CutsList viableCuts;
  for (const auto &cut : cuts) {
    if (cut.second) {
      viableCuts.push_back(cut.first);
    }
  }
  return viableCuts;
}

bool CutExtractor::cutDominates(const Cut &cut1, const Cut &cut2) const {
  if (cut1.entryIdxs.size() > cut2.entryIdxs.size()) {
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

} // namespace eda::gate::optimizer2
