//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "rewriter.h"

#include <limits>

namespace eda::gate::optimizer {

void Rewriter::transform(SubnetBuilder &builder) const {

  CutExtractor cutExtractor(&builder, k);
  std::function cutRecompute = [&cutExtractor](const size_t entryID) {
    cutExtractor.recomputeCuts(entryID);
  };
  for (SafePasser iter(builder.begin(), &cutRecompute);
       iter != builder.end() && !builder.getCell(*iter).isOut();
       ++iter) {
    rewriteOnNode(builder, iter, cutExtractor);
  }
}

void Rewriter::rewriteOnNode(
    SubnetBuilder &builder,
    SafePasser &iter,
    CutExtractor &cutExtractor) const {

  const size_t entryID = *iter;
  const auto &cuts = cutExtractor.getCuts(entryID);
  float bestMetricValue = std::numeric_limits<float>::lowest();
  SubnetID bestRhsID = 0;
  std::unordered_map<size_t, size_t> bestRhsToLhs;

  for (const auto &cut : cuts) {
    const SubnetWindow window(builder, cut);
    const SubnetID rhsID = resynthesizer.resynthesize(window);
    if (rhsID == model::OBJ_NULL_ID) {
      continue;
    }
    const Subnet &rhs = Subnet::get(rhsID);
    const auto rhsToLhs = window.getInOutMapping(rhs); // FIXME: Deprecated.
    float curMetricValue = cost(builder.evaluateReplace(rhsID, rhsToLhs));
    if (curMetricValue - bestMetricValue > metricEps) {
      bestMetricValue = curMetricValue;
      bestRhsID = rhsID;
      bestRhsToLhs = rhsToLhs;
    }
  }
  std::function cutRecompute = [&cutExtractor](const size_t entryID) {
    cutExtractor.recomputeCuts(entryID);
  };
  if (bestMetricValue > metricEps ||
      (zero_cost && std::fabs(bestMetricValue) <= metricEps)) {
    iter.replace(bestRhsID, bestRhsToLhs, nullptr, &cutRecompute, &cutRecompute,
                 &cutRecompute);
  }
}

} // namespace eda::gate::optimizer
