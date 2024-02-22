//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer2/reconvergence_cut.h"

namespace eda::gate::optimizer2 {

unsigned computeCost(const model::Subnet &subnet,
                     size_t idx,
                     const std::unordered_set<size_t> &visited) {

  const auto &entries = subnet.getEntries();
  const auto &cell = entries[idx].cell;

  if (cell.isIn() || cell.isZero() || cell.isOne()) {
    return -1;
  }

  unsigned cost = 0;

  const auto links = subnet.getLinks(idx);
  for (const auto &link : links) {
    if (visited.find(link.idx) == visited.end()) {
      cost++;
    }
  }
  return cost;
}

std::vector<size_t> getReconvergenceCut(const model::Subnet &subnet,
                                        const std::vector<size_t> &roots,
                                        size_t cutSize) {

  assert(roots.size() <= cutSize && "Number of roots more than cut size");

  std::unordered_set<size_t> visited(roots.begin(), roots.end());
  std::vector<size_t> leaves(roots.begin(), roots.end());
  leaves.reserve(cutSize);

  const auto &entries = subnet.getEntries();

  // Construct a cut.
  while (true) {
    const unsigned worstCost = -1;
    unsigned bestCost = worstCost;
    size_t bestLeave = 0;
    // Choose a leave with the best cost.
    for (size_t i = 0; i < leaves.size(); ++i) {
      const unsigned cost = computeCost(subnet, leaves[i], visited);
      if (cost < bestCost) {
        bestCost = cost;
        bestLeave = i;
      }
      if (bestCost == 0) {
        break;
      }
    }

    if ((bestCost == worstCost) || (bestCost + leaves.size() - 1 > cutSize)) {
      return leaves;
    }
    // Replace the best leave with it inputs.
    const auto links = subnet.getLinks(leaves[bestLeave]);
    leaves.erase(leaves.begin() + bestLeave);
    for (const auto &link : links) {
      const auto &id = link.idx;
      const auto &cell = entries[id].cell;

      bool constant = (cell.isZero() || cell.isOne());
      if (!constant && (visited.find(id) == visited.end())) {
        visited.emplace(id);
        leaves.push_back(id);
      }
    }
  }
}

} // namespace eda::gate::optimizer2
