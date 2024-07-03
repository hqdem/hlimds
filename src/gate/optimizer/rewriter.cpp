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
  CutExtractor cutExtractor(&builder, k, false);
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
  SubnetObject bestRhs{};
  InOutMapping bestMap;

  for (const auto &cut : cuts) {
    SubnetView window(builder, cut);
    SubnetObject rhs = resynthesizer.resynthesize(window);
    if (rhs.isNull()) {
      continue;
    }
    const auto rhsToLhs = window.getInOutMapping();
    float curMetricValue = cost(builder.evaluateReplace(rhs, rhsToLhs));
    if (curMetricValue - bestMetricValue > metricEps) {
      bestMetricValue = curMetricValue;
      bestRhs = std::move(rhs);
      bestMap = rhsToLhs;
    }
  }
  std::function cutRecompute = [&cutExtractor](const size_t entryID) {
    cutExtractor.recomputeCuts(entryID);
  };
  if (bestMetricValue > metricEps ||
      (zero_cost && std::fabs(bestMetricValue) <= metricEps)) {
    iter.replace(bestRhs, bestMap, &cutRecompute, &cutRecompute, &cutRecompute);
  }
}

} // namespace eda::gate::optimizer
