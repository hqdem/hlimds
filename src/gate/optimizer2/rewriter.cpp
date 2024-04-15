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
  for (auto iter = builder.begin(); iter != builder.end(); ) {
    const auto entryID = *iter;
    if (builder.getEntry(entryID).cell.isOut()) {
      iter++;
      continue;
    }
    cutExtractor.recomputeCuts(entryID);
    auto tmpIter = iter;
    ++tmpIter;
    rewriteOnNode(builder, entryID, cutExtractor);
    iter = tmpIter;
  }
}

void Rewriter::rewriteOnNode(
    SubnetBuilder &builder,
    const size_t entryID,
    CutExtractor &cutExtractor) const {

  ConeBuilder coneBuilder(&builder);
  const auto &cuts = cutExtractor.getCuts(entryID);
  int bestMetricValue = 0;
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
    builder.replace(bestRhsID, bestRhsToLhs, nullptr, &cutRecompute);
  }
}

} // namespace eda::gate::optimizer2
