//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/mffc.h"

namespace eda::gate::optimizer {

using Builder  = model::SubnetBuilder;
using Cell     = model::Subnet::Cell;
using Fragment = SubnetIteratorBase::SubnetFragment;
using Link     = model::Subnet::Link;
using LinkList = model::Subnet::LinkList;
using Nodes    = std::vector<size_t>;
using IdxMap   = std::unordered_map<size_t, size_t>;

static void mapCell(const Builder &builder,
                    Builder &coneBuilder,
                    size_t cellId,
                    IdxMap &oldToNew) {

  const Cell &cell = builder.getCell(cellId);
  const auto links = builder.getLinks(cellId);

  LinkList inputs;
  for (const auto &link : links) {
    if (oldToNew.find(link.idx) == oldToNew.end()) {
      mapCell(builder, coneBuilder, link.idx, oldToNew);
    }
    inputs.emplace_back(oldToNew.at(link.idx), link.inv);
  }

  const size_t idx = coneBuilder.addCell(cell.getSymbol(), inputs).idx;
  oldToNew[cellId] = idx;
}

static void getMffcBoundsRecursively(const Builder &builder,
                                     std::unordered_set<size_t> &bounds,
                                     size_t cellId,
                                     IdxMap &cellToRef) {

  const auto links = builder.getLinks(cellId);
  for (const auto &link : links) {
    const Cell &cell = builder.getCell(link.idx);
    if (cell.isOne() || cell.isZero()) {
      continue;
    }

    if (!cellToRef.at(link.idx)) {
      cellToRef[link.idx] = Cell::MaxRefCount + 1;
      getMffcBoundsRecursively(builder, bounds, link.idx, cellToRef);
      continue;
    }

    if (cellToRef.at(link.idx) != (Cell::MaxRefCount + 1)) {
      bounds.insert(link.idx);
    }
  }
}

static void getMffcBounds(const Builder &builder,
                          size_t root,
                          size_t nLeaves,
                          std::unordered_set<size_t> &bounds,
                          IdxMap &cellToRef) {

  bounds.clear();
  bounds.reserve(nLeaves);

  getMffcBoundsRecursively(builder, bounds, root, cellToRef);
}

static void dereferenceCells(const Builder &builder,
                             size_t cellId,
                             IdxMap &cellToRef,
                             const IdxMap &oldToNew) {

  const auto links = builder.getLinks(cellId);
  for (const auto &link : links) {
    const Cell &cell = builder.getCell(link.idx);
    if (cellToRef.find(link.idx) == cellToRef.end()) {
      cellToRef[link.idx] = cell.refcount;
    }

    if (oldToNew.find(link.idx) != oldToNew.end()) {
      continue;
    }

    --cellToRef[link.idx];
    if (!cellToRef.at(link.idx)) {
      dereferenceCells(builder, link.idx, cellToRef, oldToNew);
    }
  }
}

static std::unordered_set<size_t> findMffcBounds(const Builder &builder,
                                                 size_t root,
                                                 size_t nLeaves,
                                                 const IdxMap &oldToNew) {

  IdxMap cellToRef;
  std::unordered_set<size_t> bounds;

  dereferenceCells(builder, root, cellToRef, oldToNew);
  getMffcBounds(builder, root, nLeaves, bounds, cellToRef);

  return bounds;
}

Fragment getMffc(const Builder &builder, size_t root, const Nodes &leaves) {
  Fragment cone;
  cone.subnetID = model::OBJ_NULL_ID;

  const size_t nLeaves = leaves.size();
  if (nLeaves < 2) {
    cone.subnetID = getReconvergenceCone(builder, root, nLeaves, cone.entryMap);
    return cone;
  }

  IdxMap oldToNew;
  for (size_t i = 0; i < nLeaves; ++i) {
    oldToNew[leaves[i]] = i;
  }

  std::unordered_set<size_t> coneBounds;
  coneBounds = findMffcBounds(builder, root, nLeaves, oldToNew);
  auto it = coneBounds.begin();
  for (size_t i = 0; i < coneBounds.size(); ++i, ++it) {
    oldToNew[(*it)] = i;
    cone.entryMap[i] = *it;
  }

  Builder coneBuilder;
  coneBuilder.addInputs(coneBounds.size());
  mapCell(builder, coneBuilder, root, oldToNew);
  size_t outId = coneBuilder.addOutput(Link(oldToNew.at(root))).idx;

  cone.entryMap[outId] = root;
  cone.subnetID = coneBuilder.make();

  return cone;
}

} // namespace eda::gate::optimizer
