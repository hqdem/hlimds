//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer2/cut_extractor.h"

namespace eda::gate::optimizer2 {

CutExtractor::CutExtractor(const Subnet &subnet, const unsigned int k) {
  const auto &entries = subnet.getEntries();
  for (std::size_t i = 0; i < entries.size(); ++i) {
    cellsCuts.push_back(findCuts(subnet, i, entries[i].cell.arity, k));
  }
};

unsigned short countSetBits(unsigned long long x) {
  x = x - ((x >> 1) & 0x5555555555555555UL);
  x = (x & 0x3333333333333333UL) + ((x >> 2) & 0x3333333333333333UL);
  return (unsigned short)((((x + (x >> 4)) & 0xF0F0F0F0F0F0F0FUL) *
                           0x101010101010101UL) >> 56);
}

const CutExtractor::CutsList &CutExtractor::getCuts(uint64_t cellIdx) const {
  return cellsCuts[cellIdx];
}

CutExtractor::CutsCells CutExtractor::getCutsCells(uint64_t cellIdx) const {
  CutsCells cutsCells;
  for (const auto &cut : getCuts(cellIdx)) {
    cutsCells.push_back(cut.cellIdxs);
  }
  return cutsCells;
}

CutExtractor::CutsList CutExtractor::findCuts(const Subnet &subnet,
                                              const std::size_t cellIdx,
                                              const uint64_t cellArity,
                                              const unsigned int k) const {

  RawCutsList cuts;
  cuts.push_back({ getOneElemCut(subnet, cellIdx), true });
  if (!cellArity) {
    return getViable(cuts);
  }
  unsigned long long cutsCombinationsN = 1;
  std::vector<std::size_t> suffCutsCombinationsN(cellArity);
  for (long long i = cellArity - 1; i >= 0; --i) {
    cutsCombinationsN *= cellsCuts[subnet.getLink(cellIdx, i).idx].size();
    suffCutsCombinationsN[i] = cutsCombinationsN;
  }
  for (std::size_t i = 0; i < cutsCombinationsN; ++i) {
    addCut(subnet, k, cellIdx, cellArity, i, cuts, suffCutsCombinationsN);
  }
  return getViable(cuts);
}

void CutExtractor::addCut(const Subnet &subnet, const unsigned int k,
                          const std::size_t cellIdx, const uint64_t cellArity,
                          unsigned long long cutsCombinationIdx,
                          RawCutsList &addedCuts,
                          const std::vector<std::size_t> &suffCutsCombN) const {

  bool newCutTooLarge = false;
  Cut newCut;
  for (std::size_t j = 0; j < cellArity; ++j) {
    std::size_t inCellIdx = subnet.getLink(cellIdx, j).idx;
    std::size_t inCellCutIdx = cutsCombinationIdx;
    if (j + 1 != cellArity) {
      inCellCutIdx = cutsCombinationIdx / suffCutsCombN[j + 1];
      cutsCombinationIdx %= suffCutsCombN[j + 1];
    }
    const Cut &cutToUnite = cellsCuts[inCellIdx][inCellCutIdx];

    if (newCut.cellIdxs.size() > k ||
        countSetBits(newCut.signature | cutToUnite.signature) > k) {

      newCutTooLarge = true;
      break;
    }
    uniteCut(newCut, cutToUnite);
  }
  if (!newCutTooLarge && checkViable(newCut, addedCuts, k)) {
    addedCuts.push_back({ newCut, true });
  }
}

void CutExtractor::uniteCut(Cut &cut1, const Cut &cut2) const {
  for (const auto &cellID : cut2.cellIdxs) {
    cut1.cellIdxs.insert(cellID);
  }
  cut1.signature |= cut2.signature;
}

CutExtractor::Cut CutExtractor::getOneElemCut(const Subnet &subnet,
                                              const std::size_t cellIdx) const {

  Cut cut;
  cut.signature = (uint64_t)1 << (cellIdx % 64);
  cut.cellIdxs = { cellIdx };
  return cut;
}

bool CutExtractor::checkViable(const Cut &cut, RawCutsList &cuts,
                               const unsigned int k) const {

  if (cut.cellIdxs.size() > k) {
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
  if (cut1.cellIdxs.size() > cut2.cellIdxs.size()) {
    return false;
  }
  if ((cut1.signature | cut2.signature) != cut2.signature) {
    return false;
  }
  for (const auto &cut1Cell : cut1.cellIdxs) {
    if (cut2.cellIdxs.find(cut1Cell) == cut2.cellIdxs.end()) {
      return false;
    }
  }
  return true;
}

} // namespace eda::gate::optimizer2
