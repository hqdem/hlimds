//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/reconvergence.h"

namespace eda::gate::optimizer {

using SubnetBuilder = eda::gate::model::SubnetBuilder;
using SubnetBuilderPtr = std::shared_ptr<gate::model::SubnetBuilder>;

static unsigned computeCost(SubnetBuilder &builder, model::EntryID idx) {
  const auto &cell = builder.getCell(idx);

  if (cell.isIn() || cell.isZero() || cell.isOne()) {
    return -1;
  }

  unsigned cost = 0;

  for (const auto &link : builder.getLinks(idx)) {
    const auto &cell = builder.getCell(link.idx);
    bool constant = cell.isZero() || cell.isOne();
    if (!constant && !builder.isMarked(link.idx)) {
      cost++;
    }
  }
  return cost;
}

static model::EntryID findBestLeave(SubnetBuilder &builder,
                                    const model::EntryIDList &leaves,
                                    uint16_t cutSize) {

  const unsigned worstCost = -1;
  unsigned bestCost = worstCost;
  model::EntryID bestLeave = 0;
  // Choose a leave with the best cost.
  for (size_t i = 0; i < leaves.size(); ++i) {
    const unsigned cost = computeCost(builder, leaves[i]);
    if (cost < bestCost) {
      bestCost = cost;
      bestLeave = i;
    }
    if (bestCost == 0) {
      break;
    }
  }

  if ((bestCost == worstCost) || (bestCost + leaves.size() - 1 > cutSize)) {
    return -1;
  }

  return bestLeave;
}

model::SubnetView getReconvergentCut(const SubnetBuilderPtr &builder,
                                     const model::EntryIDList &roots,
                                     uint16_t cutSize) {

  assert(roots.size() <= cutSize && "Number of roots more than the cut size");

  model::EntryIDList leaves(roots.begin(), roots.end());
  leaves.reserve(cutSize + 1);

  builder->startSession();
  // Construct a cut.
  while (true) {
    auto bestLeave = findBestLeave(*builder, leaves, cutSize);
    if (bestLeave == (model::EntryID)-1) {
      builder->endSession();
      // Case when there are only constant inputs.
      if (leaves.empty()) {
        return model::SubnetView{builder, {roots, roots}};
      }
      return model::SubnetView{builder, {leaves, roots}};
    }
    // Replace the best leave with it inputs.
    for (const auto &link : builder->getLinks(leaves[bestLeave])) {
      const auto &cell = builder->getCell(link.idx);
      bool constant = (cell.isZero() || cell.isOne());
      if (!constant && !builder->isMarked(link.idx)) {
        builder->mark(link.idx);
        leaves.push_back(link.idx);
      }
    }
    leaves.erase(leaves.begin() + bestLeave);
  }
  builder->endSession();
  return model::SubnetView{builder, {leaves, roots}};
}

} // namespace eda::gate::optimizer
