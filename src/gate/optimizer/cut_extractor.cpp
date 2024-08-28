//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/cut_extractor.h"

namespace eda::gate::optimizer {

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
  entriesCuts.reserve(static_cast<size_t>(1.25 * (builder->getMaxIdx() + 1)));

  // Cuts might be (re)computed on demand.
  if (extractNow) {
    entriesCuts.resize(builder->getMaxIdx() + 1);
    for (auto it = builder->begin(); it != builder->end(); it.nextCell()) {
      findCuts(*it);
    }
  }
}

void CutExtractor::recomputeCuts(const model::EntryID entryID) {
  if (entriesCuts.size() <= entryID) {
    entriesCuts.resize(entryID + 1);
  }
  entriesCuts[entryID].clear();
  findCuts(entryID);
}

CutExtractor::CutsEntries CutExtractor::getCutsEntries(
    const model::EntryID entryID) const {
  CutsEntries cutsEntries;
  for (const auto &cut : getCuts(entryID)) {
    cutsEntries.push_back(cut.leafIDs);
  }
  return cutsEntries;
}

void CutExtractor::findCuts(const model::EntryID entryID) {
  uint16_t nLinks;
  const auto *links = getLinks(entryID, nullptr, nLinks);

  RawCutsList cuts;

  if (nLinks == 0) {
    cuts.emplace_back(Cut(k, entryID, true /* immutable */), true);
    addViableCuts(cuts, entryID);
    return;
  }

  uint64_t cutsCombinationsN = 1;
  std::vector<uint64_t> suffCutsCombinationsN(nLinks);
  for (int i = nLinks - 1; i >= 0; --i) {
    cutsCombinationsN *= entriesCuts[links[i].idx].size();
    suffCutsCombinationsN[i] = cutsCombinationsN;
  }

  cuts.reserve(cutsCombinationsN + 1);
  cuts.emplace_back(Cut(k, entryID, true /* immutable */), true);

  for (uint64_t i = 0; i < cutsCombinationsN; ++i) {
    addCut(entryID, links, nLinks, i, cuts, suffCutsCombinationsN);
  }

  addViableCuts(cuts, entryID);
}

void CutExtractor::addCut(
    const model::EntryID entryID,
    const Link links[],
    const uint16_t nLinks,
    uint64_t cutsCombinationID,
    RawCutsList &addedCuts,
    const std::vector<uint64_t> &suffCutsCombN) const {
  Cut newCut(entryID, Cut::Set(k /* empty k-set */, false /* modifiable */));

  for (uint16_t j = 0; j < nLinks; ++j) {
    model::EntryID inputID = links[j].idx;
    size_t inputCutIndex = cutsCombinationID;
    if (j + 1 != nLinks) {
      inputCutIndex = cutsCombinationID / suffCutsCombN[j + 1];
      cutsCombinationID %= suffCutsCombN[j + 1];
    }
    const Cut &cutToMerge = entriesCuts[inputID][inputCutIndex];
    if (!newCut.merge(cutToMerge)) {
      return;
    }
  }
  if (cutNotDominated(newCut, addedCuts)) {
    addedCuts.emplace_back(std::move(newCut), true);
  }
}

void CutExtractor::addViableCuts(
    const RawCutsList &cuts, const model::EntryID entryID) {
  entriesCuts[entryID].clear();

  for (const auto &cut : cuts) {
    if (cut.second) {
      auto &cutEntry = entriesCuts[entryID];
      cutEntry.push_back(cut.first);
    }
  }
}

bool CutExtractor::cutNotDominated(const Cut &cut, RawCutsList &cuts) const {
  for (size_t i = 0; i < cuts.size(); ++i) {
    if (cuts[i].first.dominates(cut)) {
      return false;
    }
    if (!cuts[i].second) {
      continue;
    }
    if (cut.dominates(cuts[i].first)) {
      cuts[i].second = false;
    }
  }
  return true;
}

} // namespace eda::gate::optimizer
