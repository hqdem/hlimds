//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "rewriter.h"

namespace eda::gate::optimizer2 {

void Rewriter::transform(SubnetBuilder &builder) const {

  CutExtractor cutExtractor(&builder, k);
  for (SafePasser iter = builder.begin();
       iter != builder.end() && !builder.getCell(*iter).isOut();
       ++iter) {
    const auto entryID = *iter;
    cutExtractor.recomputeCuts(entryID);
    rewriteOnNode(builder, iter, cutExtractor);
  }
}

void Rewriter::rewriteOnNode(
    SubnetBuilder &builder,
    SafePasser &iter,
    CutExtractor &cutExtractor) const {

  const size_t entryID = *iter;
  ConeBuilder coneBuilder(&builder);
  const auto &cuts = cutExtractor.getCuts(entryID);
  int bestMetricValue = INT32_MIN;
  SubnetID bestRhsID = 0;
  std::unordered_map<size_t, size_t> bestRhsToLhs;

  for (const auto &cut : cuts) {
    const auto &cone = coneBuilder.getCone(cut);
    const auto &coneSubnet = Subnet::get(cone.subnetID);
    const SubnetID rhsID = resynthesizer.resynthesize(cone.subnetID);
    if (rhsID == model::OBJ_NULL_ID) {
      continue;
    }
    const Subnet &rhs = Subnet::get(rhsID);
    std::unordered_map<size_t, size_t> rhsToLhs;
    for (size_t i = 0; i < rhs.getInNum(); ++i) {
      rhsToLhs[i] = cone.coneEntryToOrig[i];
    }
    for (size_t i = 1; i <= rhs.getOutNum(); ++i) {
      rhsToLhs[rhs.getEntries().size() - i] =
          cone.coneEntryToOrig[coneSubnet.getEntries().size() - i];
    }
    int curMetricValue = builder.evaluateReplace(rhsID, rhsToLhs).size;
    if (curMetricValue > bestMetricValue) {
      bestMetricValue = curMetricValue;
      bestRhsID = rhsID;
      bestRhsToLhs = rhsToLhs;
    }
  }
  std::function cutRecompute = [&cutExtractor](const size_t entryID) {
    cutExtractor.recomputeCuts(entryID);
  };
  if (bestMetricValue > 0) {
    iter.replace(bestRhsID, bestRhsToLhs, nullptr, &cutRecompute, &cutRecompute,
                 &cutRecompute);
  }
}

} // namespace eda::gate::optimizer2
