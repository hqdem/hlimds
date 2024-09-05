//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "subnet_techmapper_pcut.h"

#include <algorithm>
#include <cassert>

namespace eda::gate::techmapper {

#define UTOPIA_CUT_PROVIDER_LAMBDA()\
  [this](const model::SubnetBuilder &builder,\
         const model::EntryID entryID) -> optimizer::CutsList {\
    computePriorityCuts(entryID);\
    return cutExtractor->getCuts(entryID);\
  }

SubnetTechMapperPCut::SubnetTechMapperPCut(
    const std::string &name,
    const criterion::Criterion &criterion,
    const uint16_t maxCutSize,
    const uint16_t maxCutNum,
    const MatchFinder matchFinder,
    const CellEstimator cellEstimator,
    const CostAggregator costAggregator,
    const CostPropagator costPropagator):
    SubnetTechMapperBase(name,
                         criterion,
                         UTOPIA_CUT_PROVIDER_LAMBDA(),
                         matchFinder,
                         cellEstimator,
                         costAggregator,
                         costPropagator),
    maxCutSize(maxCutSize), maxCutNum(maxCutNum) {}

SubnetTechMapperPCut::SubnetTechMapperPCut(
    const std::string &name,
    const criterion::Criterion &criterion,
    const uint16_t maxCutSize,
    const uint16_t maxCutNum,
    const MatchFinder matchFinder,
    const CellEstimator cellEstimator):
    SubnetTechMapperBase(name,
                         criterion,
                         UTOPIA_CUT_PROVIDER_LAMBDA(),
                         matchFinder,
                         cellEstimator),
    maxCutSize(maxCutSize), maxCutNum(maxCutNum) {}

void SubnetTechMapperPCut::computePriorityCuts(const model::EntryID entryID) {
  const size_t n = maxCutNum + 1 /* trivial cut */;

  cutExtractor->recomputeCuts(entryID);
  if (cutExtractor->getCutNum(entryID) <= n) return;

  auto cuts = cutExtractor->getCuts(entryID);
  std::vector<std::pair<size_t, criterion::Cost>> sorted(cuts.size());

  for (size_t i = 0; i < cuts.size(); ++i) {
    const auto &cut = cuts[i];

    criterion::Cost cost;
    if (cut.isTrivial()) {
      cost = 0. /* trivial cut must be included */;
    } else {
      const auto leafVectors = getCostVectors(cut);
      const auto aggregation = costAggregator(leafVectors);
      cost = criterion.getPenalizedCost(aggregation, tension);
    }

    sorted[i] = {i, cost};
  }

  std::sort(sorted.begin(), sorted.end(), [](const auto &lhs, const auto &rhs) {
    return lhs.second < rhs.second;
  });

  optimizer::CutsList pcuts;
  pcuts.reserve(n);

  for (size_t i = 0; i < n; ++i) {
    pcuts.push_back(cuts[sorted[i].first]);
  }

  cutExtractor->setCuts(entryID, pcuts);
}

} // namespace eda::gate::techmapper
