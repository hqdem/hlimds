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

void Rewriter::transform(const std::shared_ptr<SubnetBuilder> &builder) const {
  SubnetBuilder *builderPtr = builder.get();
  CutExtractor cutExtractor(builderPtr, k, false);
  std::function cutRecompute = [&cutExtractor](const size_t entryID) {
    cutExtractor.recomputeCuts(entryID);
  };
  std::function cutRecomputeDepthCond = [&cutExtractor](const size_t entryID,
      const size_t oldRootDepth, const size_t curEntryDepth) {

    if (curEntryDepth <= oldRootDepth) {
      cutExtractor.recomputeCuts(entryID);
    }
  };
  for (SafePasser iter(builderPtr->begin(), &cutRecompute);
       !builderPtr->getCell(*iter).isOut();
       ++iter) {
    rewriteOnNode(*builderPtr, iter, cutExtractor, &cutRecompute,
        &cutRecomputeDepthCond);
  }
}

void Rewriter::rewriteOnNode(
    SubnetBuilder &builder,
    SafePasser &iter,
    CutExtractor &cutExtractor,
    const CellActionCallback *cutRecompute,
    const CellCallbackCondition *cutRecomputeDepthCond) const {
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
  if (bestMetricValue > metricEps ||
      (zeroCost && std::fabs(bestMetricValue) <= metricEps)) {
    iter.replace(bestRhs, bestMap, cutRecompute, cutRecompute, cutRecompute,
                 cutRecomputeDepthCond);
  }
}

} // namespace eda::gate::optimizer
