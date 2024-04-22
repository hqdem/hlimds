//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer2/area_subnet_iterator.h"

namespace eda::gate::optimizer2 {

using Cell          = eda::gate::model::Subnet::Cell;
using IdxMap        = std::unordered_map<size_t, size_t>;
using Link          = eda::gate::model::Subnet::Link;
using LinkList      = eda::gate::model::Subnet::LinkList;
using SubnetBuilder = eda::gate::model::SubnetBuilder;
using SubnetID      = gate::model::SubnetID;

void mapCell(const SubnetBuilder &subnetBuilder,
             SubnetBuilder &coneBuilder,
             size_t cellId,
             IdxMap &oldToNew) {

  const Cell &cell = subnetBuilder.getCell(cellId);
  const auto links = subnetBuilder.getLinks(cellId);

  LinkList inputs;
  for (const auto &link : links) {
    if (oldToNew.find(link.idx) == oldToNew.end()) {
      mapCell(subnetBuilder, coneBuilder, link.idx, oldToNew);
    }
    inputs.emplace_back(oldToNew.at(link.idx), link.inv);
  }
  const size_t idx = coneBuilder.addCell(cell.getSymbol(), inputs).idx;
  oldToNew[cellId] = idx;
}

void getMffcBoundsRec(const SubnetBuilder &subnetBuilder,
                      std::unordered_set<size_t> &bounds,
                      size_t cellId,
                      IdxMap &cellToRef) {

  const auto links = subnetBuilder.getLinks(cellId);
  for (const auto &link : links) {
    const Cell &cell = subnetBuilder.getCell(link.idx);
    if (cell.isOne() || cell.isZero()) {
      continue;
    }

    if (!cellToRef.at(link.idx)) {
      cellToRef[link.idx] = Cell::MaxRefCount + 1;
      getMffcBoundsRec(subnetBuilder, bounds, link.idx, cellToRef);
      continue;
    }

    if (cellToRef.at(link.idx) != (Cell::MaxRefCount + 1)) {
      bounds.insert(link.idx);
    }
  }
}

void getMffcBounds(const SubnetBuilder &subnetBuilder,
                   size_t rootId,
                   size_t nIn,
                   std::unordered_set<size_t> &bounds,
                   IdxMap &cellToRef) {

  bounds.clear();
  bounds.reserve(nIn);

  getMffcBoundsRec(subnetBuilder, bounds, rootId, cellToRef);
}

void dereferenceCells(const SubnetBuilder &subnetBuilder,
                      size_t cellId,
                      IdxMap &cellToRef,
                      const IdxMap &oldToNew) {

  const auto links = subnetBuilder.getLinks(cellId);
  for (const auto &link : links) {
    const Cell &cell = subnetBuilder.getCell(link.idx);
    if (cellToRef.find(link.idx) == cellToRef.end()) {
      cellToRef[link.idx] = cell.refcount;
    }

    if (oldToNew.find(link.idx) != oldToNew.end()) {
      continue;
    }

    --cellToRef[link.idx];
    if (!cellToRef.at(link.idx)) {
      dereferenceCells(subnetBuilder, link.idx, cellToRef, oldToNew);
    }
  }
}

std::unordered_set<size_t> findMffcBounds(const SubnetBuilder &subnetBuilder,
                                          size_t rootId,
                                          size_t nIn,
                                          const std::vector<size_t> &leaves,
                                          const IdxMap &oldToNew) {

  IdxMap cellToRef;
  std::unordered_set<size_t> bounds;

  dereferenceCells(subnetBuilder, rootId, cellToRef, oldToNew);
  getMffcBounds(subnetBuilder, rootId, nIn, bounds, cellToRef);

  return bounds;
}

SubnetID getMffc(const SubnetBuilder &subnetBuilder,
                 size_t rootId,
                 size_t nIn,
                 const std::vector<size_t> &leaves,
                 IdxMap &newToOld) {

  if (leaves.size() <= 2) {
    return getReconvCone(subnetBuilder, rootId, nIn, newToOld);
  }

  IdxMap oldToNew;
  for (size_t i = 0; i < leaves.size(); ++i) {
    oldToNew[leaves[i]] = i;
  }

  std::unordered_set<size_t> coneBounds;
  coneBounds = findMffcBounds(subnetBuilder, rootId, nIn, leaves, oldToNew);
  auto it = coneBounds.begin();
  for (size_t i = 0; i < coneBounds.size(); ++i, ++it) {
    oldToNew[(*it)] = i;
    newToOld[i] = *it;
  }

  SubnetBuilder coneBuilder;
  coneBuilder.addInputs(coneBounds.size());
  mapCell(subnetBuilder, coneBuilder, rootId, oldToNew);
  size_t outId = coneBuilder.addOutput(Link(oldToNew.at(rootId))).idx;

  newToOld[outId] = rootId;

  return coneBuilder.make();
}

SubnetIteratorBase::SubnetFragment AreaSubnetIterator::next() {
  SubnetFragment sf;
  sf.subnetID = model::OBJ_NULL_ID;

  ++iter;
  const auto rootId = *iter;
  if (subnetBuilder.getCell(rootId).isOut()) {
    return sf;
  }

  auto leaves = getReconvCut(subnetBuilder, rootId, nIn);
  sf.subnetID = getMffc(subnetBuilder, rootId, nIn, leaves, sf.entryMap);

  return sf;
}

} // namespace eda::gate::optimizer2
