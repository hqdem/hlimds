//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/reconvergence_cut.h"

namespace eda::gate::optimizer {

using IdxMap        = std::unordered_map<size_t, size_t>;
using Link          = eda::gate::model::Subnet::Link;
using SubnetBuilder = eda::gate::model::SubnetBuilder;

static void buildFromRoot(SubnetBuilder &coneBuilder,
                          const SubnetBuilder &builder,
                          size_t idx,
                          IdxMap &map) {

  if (map.find(idx) != map.end()) {
    return;
  }
  auto links = builder.getLinks(idx);
  auto symbol = builder.getCell(idx).getSymbol();
  for (size_t i = 0; i < links.size(); ++i) {
    if (map.find(links[i].idx) == map.end()) {
      buildFromRoot(coneBuilder, builder, links[i].idx, map);
    }
    size_t idNew = map.at(links[i].idx);
    links[i].idx = idNew;
  }
  map[idx] = coneBuilder.addCell(symbol, links).idx;
}

static unsigned computeCost(SubnetBuilder &builder, size_t idx) {
  const auto &cell = builder.getCell(idx);

  if (cell.isIn() || cell.isZero() || cell.isOne()) {
    return -1;
  }

  const auto sessionID = builder.getSession();
  unsigned cost = 0;

  for (const auto &link : builder.getLinks(idx)) {
    const auto &cell = builder.getCell(link.idx);
    bool constant = cell.isZero() || cell.isOne();
    if (!constant && (sessionID != builder.getEntrySession(link.idx))) {
      cost++;
    }
  }
  return cost;
}

static size_t findBestLeave(SubnetBuilder &builder,
                            const std::vector<size_t> &leaves,
                            uint16_t cutSize) {

  const unsigned worstCost = -1;
  unsigned bestCost = worstCost;
  size_t bestLeave = 0;
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

std::vector<size_t> getReconvergenceCut(SubnetBuilder &builder,
                                        const std::vector<size_t> &roots,
                                        uint16_t cutSize) {

  assert(roots.size() <= cutSize && "Number of roots more than the cut size");

  std::vector<size_t> leaves(roots.begin(), roots.end());
  leaves.reserve(cutSize + 1);

  builder.startSession();
  const auto sessionID = builder.getSession();
  // Construct a cut.
  while (true) {
    size_t bestLeave = findBestLeave(builder, leaves, cutSize);
    if (bestLeave == (size_t)-1) {
      builder.endSession();
      // Case when there are only constant inputs.
      if (leaves.empty()) {
        return roots;
      }
      return leaves;
    }
    // Replace the best leave with it inputs.
    for (const auto &link : builder.getLinks(leaves[bestLeave])) {
      const auto &cell = builder.getCell(link.idx);
      bool constant = (cell.isZero() || cell.isOne());
      if (!constant && (sessionID != builder.getEntrySession(link.idx))) {
        builder.markEntry(link.idx);
        leaves.push_back(link.idx);
      }
    }
    leaves.erase(leaves.begin() + bestLeave);
  }
  builder.endSession();
  return leaves;
}

model::SubnetID getReconvergenceWindow(SubnetBuilder &builder,
                                       const std::vector<size_t> &roots,
                                       uint16_t cutSize,
                                       IdxMap &map) {

  assert(roots.size() <= cutSize && "Number of roots more than the cut size");

  std::vector<size_t> leaves = getReconvergenceCut(builder, roots, cutSize);

  map.clear();
  SubnetBuilder coneBuilder;
  IdxMap mapping;
  // Case when there are only constant inputs or the roots are constant or INs.
  bool equal = leaves == roots;
  for (size_t i = 0; i < leaves.size(); ++i) {
    if (!equal || (equal && builder.getCell(leaves[i]).isIn())) {
      coneBuilder.addInput();
      mapping[leaves[i]] = i;
      map[i] = leaves[i];
    }
  }
  // Case when there are only constant inputs.
  if (map.empty()) {
    mapping[0] = 0;
    map[0] = 0;
    coneBuilder.addInput();
  }
  for (const auto &root : roots) {
    buildFromRoot(coneBuilder, builder, root, mapping);
  }
  for (const auto &root : roots) {
    const size_t outId = coneBuilder.addOutput(Link(mapping.at(root))).idx;
    map[outId] = root;
  }
  return coneBuilder.make();
}

} // namespace eda::gate::optimizer
