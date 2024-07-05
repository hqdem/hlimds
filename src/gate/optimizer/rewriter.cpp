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

void Rewriter::transform(const SubnetBuilderPtr &builder) const {
  SubnetBuilder *builderPtr = builder.get();
  CutExtractor cutExtractor(builderPtr, k, false);
  std::function cutRecompute = [&cutExtractor](const size_t entryID) {
    cutExtractor.recomputeCuts(entryID);
  };
  for (SafePasser iter(builderPtr->begin(), &cutRecompute);
       iter != builderPtr->end() && !builderPtr->getCell(*iter).isOut();
       ++iter) {
    rewriteOnNode(*builderPtr, iter, cutExtractor);
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
  InOutMapping bestMap{};

  for (const auto &cut : cuts) {
    SubnetView cone(builder, cut);
    SubnetObject rhs = resynthesizer.resynthesize(cone);
    if (rhs.isNull()) {
      continue;
    }
    const auto rhsToLhs = cone.getInOutMapping();
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
      (zeroCost && std::fabs(bestMetricValue) <= metricEps)) {
    iter.replace(bestRhs, bestMap, &cutRecompute, &cutRecompute, &cutRecompute);
  }
}

} // namespace eda::gate::optimizer
