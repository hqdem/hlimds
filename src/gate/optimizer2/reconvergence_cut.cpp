//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer2/reconvergence_cut.h"

namespace eda::gate::optimizer2 {

static void mapCell(model::SubnetBuilder &coneBuilder,
                    const model::SubnetBuilder &builder,
                    size_t idx,
                    std::unordered_map<size_t, size_t> &map) {

  if (map.find(idx) != map.end()) {
    return;
  }
  auto links = builder.getLinks(idx);
  auto symbol = builder.getCell(idx).getSymbol();
  for (size_t i = 0; i < links.size(); ++i) {
    if (map.find(links[i].idx) == map.end()) {
      mapCell(coneBuilder, builder, links[i].idx, map);
    }
    size_t idNew = map.at(links[i].idx);
    links[i].idx = idNew;
  }
  map[idx] = coneBuilder.addCell(symbol, links).idx;
}

template<typename SubnetOrBuilder>
static unsigned computeCost(const SubnetOrBuilder &subnetOrBuilder,
                            const model::Subnet::Cell &cell,
                            size_t idx,
                            const std::unordered_set<size_t> &visited) {

  if (cell.isIn() || cell.isZero() || cell.isOne()) {
    return -1;
  }

  unsigned cost = 0;

  const auto links = subnetOrBuilder.getLinks(idx);
  for (const auto &link : links) {
    if (visited.find(link.idx) == visited.end()) {
      cost++;
    }
  }
  return cost;
}

static unsigned computeCost(const model::Subnet &subnet,
                            size_t idx,
                            const std::unordered_set<size_t> &visited) {

  const auto &entries = subnet.getEntries();
  const auto &cell = entries[idx].cell;

  return computeCost(subnet, cell, idx, visited);
}

static unsigned computeCost(const model::SubnetBuilder &builder,
                            size_t idx,
                            const std::unordered_set<size_t> &visited) {

  const auto &cell = builder.getCell(idx);

  return computeCost(builder, cell, idx, visited);
}

static void replaceBestLeave(std::vector<size_t> &leaves,
                             std::unordered_set<size_t> &visited,
                             const model::Subnet::Cell &cell,
                             size_t idx) {

  bool constant = (cell.isZero() || cell.isOne());
  if (!constant && (visited.find(idx) == visited.end())) {
    visited.emplace(idx);
    leaves.push_back(idx);
  }
}

template<typename SubnetOrBuilder>
static size_t findBestLeave(const SubnetOrBuilder &subnetOrBuilder,
                            const std::vector<size_t> &leaves,
                            const std::unordered_set<size_t> &visited,
                            size_t cutSize) {

  const unsigned worstCost = -1;
  unsigned bestCost = worstCost;
  size_t bestLeave = 0;
  // Choose a leave with the best cost.
  for (size_t i = 0; i < leaves.size(); ++i) {
    const unsigned cost = computeCost(subnetOrBuilder, leaves[i], visited);
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

std::vector<size_t> getReconvergenceCut(const model::Subnet &subnet,
                                        const std::vector<size_t> &roots,
                                        size_t cutSize) {

  assert(roots.size() <= cutSize && "Number of roots more than the cut size");

  std::unordered_set<size_t> visited(roots.begin(), roots.end());
  std::vector<size_t> leaves(roots.begin(), roots.end());
  leaves.reserve(cutSize);

  const auto &entries = subnet.getEntries();

  // Construct a cut.
  while (true) {
    size_t bestLeave = findBestLeave(subnet, leaves, visited, cutSize);
    if (bestLeave == (size_t)-1) {
      // Case when there are only constant inputs
      if (leaves.empty()) {
        return roots;
      }
      return leaves;
    }
    // Replace the best leave with it inputs.
    const auto links = subnet.getLinks(leaves[bestLeave]);
    leaves.erase(leaves.begin() + bestLeave);
    for (const auto &link : links) {
      const auto &cell = entries[link.idx].cell;
      replaceBestLeave(leaves, visited, cell, link.idx);
    }
  }
}

std::vector<size_t> getReconvergenceCut(const model::SubnetBuilder &builder,
                                        const std::vector<size_t> &roots,
                                        size_t cutSize) {

  assert(roots.size() <= cutSize && "Number of roots more than the cut size");

  std::unordered_set<size_t> visited(roots.begin(), roots.end());
  std::vector<size_t> leaves(roots.begin(), roots.end());
  leaves.reserve(cutSize);

  // Construct a cut.
  while (true) {
    size_t bestLeave = findBestLeave(builder, leaves, visited, cutSize);
    if (bestLeave == (size_t)-1) {
      // Case when there are only constant inputs
      if (leaves.empty()) {
        return roots;
      }
      return leaves;
    }
    // Replace the best leave with it inputs.
    const auto links = builder.getLinks(leaves[bestLeave]);
    leaves.erase(leaves.begin() + bestLeave);
    for (const auto &link : links) {
      const auto &cell = builder.getCell(link.idx);
      replaceBestLeave(leaves, visited, cell, link.idx);
    }
  }
}

model::SubnetID getReconvergenceCone(const model::SubnetBuilder &builder,
                                     const std::vector<size_t> &roots,
                                     size_t cutSize,
                                     std::unordered_map<size_t, size_t> &map) {

  assert(roots.size() <= cutSize && "Number of roots more than the cut size");

  std::unordered_set<size_t> visited(roots.begin(), roots.end());
  std::vector<size_t> leaves(roots.begin(), roots.end());
  leaves.reserve(cutSize);

  // Construct a cut.
  while (true) {
    size_t bestLeave = findBestLeave(builder, leaves, visited, cutSize);
    if (bestLeave == (size_t)-1) {
      map.clear();
      // Delete the leaves from the set with the visited nodes
      for (const auto &inputsId : leaves) {
        visited.erase(inputsId);
      }
      // Create Cone
      model::SubnetBuilder coneBuilder;
      std::unordered_map<size_t, size_t> mapping;
      // Add inputs to map
      // Case when there are only constant inputs
      if (leaves.empty()) {
        mapping[0] = 0;
        map[0] = 0;
        coneBuilder.addInput();
      }
      for (size_t i = 0; i < leaves.size(); ++i) {
        mapping[leaves[i]] = i;
        map[i] = leaves[i];
      }
      coneBuilder.addInputs(leaves.size());
      // Add inner gates
      for (const auto &idx : visited) {
        mapCell(coneBuilder, builder, idx, mapping);
      }
      // Add outputs
      for (const auto &idx : roots) {
        size_t outId = coneBuilder.addOutput(model::Subnet::Link(mapping[idx])).idx;
        map[outId] = idx;
      }
      return coneBuilder.make();
    }
    // Replace the best leave with it inputs.
    const auto links = builder.getLinks(leaves[bestLeave]);
    leaves.erase(leaves.begin() + bestLeave);
    for (const auto &link : links) {
      const auto &cell = builder.getCell(link.idx);
      replaceBestLeave(leaves, visited, cell, link.idx);
    }
  }
}

} // namespace eda::gate::optimizer2
