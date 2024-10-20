//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "diag/logger.h"
#include "gate/techmapper/subnet_techmapper_pcut.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <tuple>

namespace eda::gate::techmapper {

#define UTOPIA_CUT_PROVIDER_LAMBDA()\
  [this](const std::shared_ptr<model::SubnetBuilder> &builder,\
         const model::EntryID entryID) -> optimizer::CutsList {\
    computePCuts(builder, entryID);\
    return cutExtractor->getCuts(entryID);\
  }

SubnetTechMapperPCut::SubnetTechMapperPCut(
    const std::string &name,
    const context::UtopiaContext &context,
    const uint16_t maxCutSize,
    const uint16_t maxCutNum,
    const CutEstimator cutEstimator,
    const MatchFinder matchFinder,
    const CellEstimator cellEstimator,
    const CostAggregator costAggregator,
    const CostPropagator costPropagator):
    SubnetTechMapperBase(name,
                         context,
                         UTOPIA_CUT_PROVIDER_LAMBDA(),
                         cutEstimator,
                         matchFinder,
                         cellEstimator,
                         costAggregator,
                         costPropagator),
    maxCutSize(maxCutSize), maxCutNum(maxCutNum) {}

SubnetTechMapperPCut::SubnetTechMapperPCut(
    const std::string &name,
    const context::UtopiaContext &context,
    const uint16_t maxCutSize,
    const uint16_t maxCutNum,
    const CutEstimator cutEstimator,
    const MatchFinder matchFinder,
    const CellEstimator cellEstimator):
    SubnetTechMapperPCut(name,
                         context,
                         maxCutSize,
                         maxCutNum,
                         cutEstimator,
                         matchFinder,
                         cellEstimator,
                         defaultCostAggregator,
                         defaultCostPropagator) {}

SubnetTechMapperPCut::SubnetTechMapperPCut(
    const std::string &name,
    const context::UtopiaContext &context,
    const uint16_t maxCutSize,
    const uint16_t maxCutNum,
    const MatchFinder matchFinder,
    const CellEstimator cellEstimator):
    SubnetTechMapperPCut(name,
                         context,
                         maxCutSize,
                         maxCutNum,
                         defaultCutEstimator,
                         matchFinder,
                         cellEstimator) {}

void SubnetTechMapperPCut::computePCuts(const SubnetBuilderPtr &builder,
                                        const model::EntryID entryID) {
  const size_t n = cutsPerCell + 1 /* trivial cut */;

  // Use good cuts from the previous try.
  constexpr size_t oldCutNum = 1;

  std::unordered_set<optimizer::Cut> goodOldCuts;
  if (tryCount /* not the first try */) {
    const auto oldCuts = cutExtractor->getCuts(entryID);

    for (const auto &oldCut : oldCuts) {
      if (!oldCut.isTrivial()) {
        goodOldCuts.insert(oldCut);
        if (oldCuts.size() == oldCutNum) break;
      }
    }
  }

  cutExtractor->recomputeCuts(entryID);
  if (cutExtractor->getCutNum(entryID) <= n) return;

  auto cuts = cutExtractor->getCuts(entryID);
  std::vector<std::vector<Match>> matches(cuts.size());
  std::vector<std::tuple<size_t, criterion::Cost, bool>> sorted(cuts.size());

  for (size_t i = 0; i < cuts.size(); ++i) {
    const auto &cut = cuts[i];

    criterion::Cost cost = cut.isTrivial()
       ? 0. /* trivial cut must be included */
       : estimateCutCost(builder, cut, true /* penalize */);

    // No matches => large cost.
    matches[i] = matchFinder(builder, cut);
    cost /= (std::log2(matches[i].size() + 1.) + .1);

    sorted[i] = {i, cost, !matches[i].empty()};
    goodOldCuts.erase(cut);
  }

  // Add good cuts from the previous try.
  for (const auto &oldCut : goodOldCuts) {
    assert(!oldCut.isTrivial());
    cuts.push_back(oldCut);
    matches.push_back(matchFinder(builder, oldCut));
    sorted.emplace_back(sorted.size(), 0. /* good */, !matches.back().empty());
  }

  std::sort(sorted.begin(), sorted.end(), [](const auto &lhs, const auto &rhs) {
    const auto lmatches = std::get<2>(lhs);
    const auto rmatches = std::get<2>(rhs);
    return (lmatches && !rmatches)
        || (lmatches == rmatches && std::get<1>(lhs) < std::get<1>(rhs));
  });

  optimizer::CutsList pcuts;
  pcuts.reserve(n);

  size_t matchCount = 0;
  for (size_t i = 0; i < n; ++i) {
    const auto index = std::get<0>(sorted[i]);
    const auto &pcut = cuts[index];

    // Store priority cuts.
    pcuts.push_back(pcut);
    // Cache the cut matches.
    cutMatches.emplace(pcut, matches[index]);

    matchCount += matches[index].size();
  }

  if (matchCount <= 1 /* trivial cut */) {
    const auto &cell = builder->getCell(entryID);
    UTOPIA_LOG_WARN("No p-cut matches found for "
        << fmt::format("cell#{}:{}", entryID, cell.getType().getName()));
  }

  cutExtractor->setCuts(entryID, pcuts);
}

} // namespace eda::gate::techmapper
