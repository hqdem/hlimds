//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "depth_subnet_iterator.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <set>

namespace eda::gate::optimizer {

size_t countNotVisitedLinks(model::Subnet::LinkList links,
                            std::unordered_set<size_t> visited) {
  int count = std::count_if(links.begin(), links.end(),
                            [&](const model::Subnet::Link& link) {
      return visited.find(link.idx) == visited.end();
  });
  return count;
}

size_t buildFragment(model::SubnetBuilder &builder,
                     const model::SubnetBuilder &subnetBuilder,
                     size_t root,
                     std::unordered_map<size_t, size_t> &entryMap) {

  using LinkList = model::StrashKey::LinkList;
  using Link = model::Subnet::Link;

  for (auto it = entryMap.begin(); it != entryMap.end(); ++it) {
    if (it->second == root) {
      return it->first;
    }
  }
  LinkList links = subnetBuilder.getLinks(root);
  for (Link &link : links) {
    link.idx = buildFragment(builder, subnetBuilder, link.idx, entryMap);
  }
  const auto& cell = subnetBuilder.getCell(root);
  Link link = builder.addCellTree(cell.getSymbol(), links, cell.arity);
  entryMap.insert({static_cast<size_t>(link.idx), root});
  return link.idx;
}

DepthSubnetIterator::SubnetFragment
    DepthSubnetIterator::getFragment(std::unordered_set<size_t> &cut,
                                     size_t root) {
  SubnetBuilder builder;
  EntryMap map;
  builder.addInputs(cut.size());
  size_t i{0};
  auto it = cut.begin();
  for (; it != cut.end(); ++i, ++it) {
    map.insert({i, *it});
  }
  auto out = buildFragment(builder, subnetBuilder, root, map);

  builder.addOutput(Link(out));
  return {builder.make(), std::move(map)};

}

DepthSubnetIterator::SubnetFragment
    DepthSubnetIterator::getCut(const SubnetBuilder &subnetBuilder,
                                size_t root,
                                size_t cutSize) {
  std::unordered_set<size_t> visited;
  std::unordered_set<size_t> leaves;
  std::unordered_set<size_t> inners;
  std::vector<size_t> cone;

  visited.insert(root);
  leaves.insert(root);
  inners.insert(root);
  cone.push_back(root);

  while (true) {
    bool findFlag = false;
    for (const auto &i: leaves) {
      const auto &links = subnetBuilder.getLinks(i);
      const auto &cell = subnetBuilder.getCell(i);

      size_t nvlinks = countNotVisitedLinks(links, visited);
      size_t lsize = leaves.size();
      // Unless there are too many descendants.
      // PI cannot be a candidate for expansion.
      if (nvlinks + lsize - 1 <= cutSize && !cell.isIn() &&
          !cell.isOne() && !cell.isZero()) {

        findFlag = true;

        leaves.erase(i);
        inners.insert(i);
        bool allConsts = true;
        for (const auto &j: links) {
          auto id = j.idx;
          const auto &entry = subnetBuilder.getEntry(id);
          const auto &cell = entry.cell;

          bool constant = (cell.isZero() || cell.isOne());
          if (!constant) {
            allConsts = false;
          }
          if (!constant && (visited.find(id) == visited.end())) {
            visited.insert(id);
            leaves.insert(id);
            cone.push_back(id);
          }
        }
        if (allConsts) {
          leaves.insert(i);
          inners.erase(i);
        }

        break;
      }
    }
    // This is in case "no suitable gate for expansion" was found.
    if (!findFlag) {
      assert(leaves.size() != 0 && "Cut size can not be null");
      return getFragment(leaves, root);
    }
  }
}

DepthSubnetIterator::SubnetFragment DepthSubnetIterator::next() {
  const auto &entry = subnetBuilder.getEntry(*start);
  auto cell = entry.cell;

  SubnetFragment nullId;
  nullId.subnetID = model::OBJ_NULL_ID;

  if ((maxCones == 0) || (cell.isOut())) {
    start = subnetBuilder.begin();
    inGate = true;
    return nullId;
  }

  if (inGate) {
    while (cell.isIn()) {
      ++start;
      cell = subnetBuilder.getEntry(*start).cell;
    }
    inGate = false;
  }

  assert(!cell.isIn() && "Try to build cone from PI!");

  auto subsubnet = getCut(subnetBuilder, *start, cutSize);
  ++start;
  --maxCones;
  return subsubnet;
}

} // namespace eda::gate::optimizer
