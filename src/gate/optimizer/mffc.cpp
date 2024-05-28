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

static uint16_t leaveNumber = 0;

static void buildFromRoot(const Builder &builder,
                          Builder &coneBuilder,
                          size_t cellId,
                          IdxMap &oldToNew) {

  const Cell &cell = builder.getCell(cellId);
  const auto links = builder.getLinks(cellId);

  LinkList inputs;
  for (const auto &link : links) {
    if (oldToNew.find(link.idx) == oldToNew.end()) {
      buildFromRoot(builder, coneBuilder, link.idx, oldToNew);
    }
    inputs.emplace_back(oldToNew.at(link.idx), link.inv);
  }

  const size_t idx = coneBuilder.addCell(cell.getSymbol(), inputs).idx;
  oldToNew[cellId] = idx;
}

static void getMffcBounds(Builder &builder,
                          size_t cellId,
                          IdxMap &cellToRef,
                          IdxMap &newToOld,
                          IdxMap &oldToNew) {

  for (const auto &link : builder.getLinks(cellId)) {
    bool mark = builder.getEntrySession(link.idx) == builder.getSession();
    const Cell &cell = builder.getCell(link.idx);
    if (mark || cell.isOne() || cell.isZero()) {
      continue;
    }
    builder.markEntry(link.idx);

    if (!cellToRef.at(link.idx)) {
      getMffcBounds(builder, link.idx, cellToRef, newToOld, oldToNew);
      continue;
    }

    newToOld[leaveNumber] = link.idx;
    oldToNew[link.idx] = leaveNumber;
    leaveNumber++;
  }
}

static void dereferenceCells(const Builder &builder,
                             size_t cellId,
                             IdxMap &cellToRef) {

  for (const auto &link : builder.getLinks(cellId)) {
    const Cell &cell = builder.getCell(link.idx);
    if (cellToRef.find(link.idx) == cellToRef.end()) {
      cellToRef[link.idx] = cell.refcount;
    }

    if (builder.getEntrySession(link.idx) == builder.getSession()) {
      continue;
    }

    --cellToRef[link.idx];
    if (!cellToRef.at(link.idx)) {
      dereferenceCells(builder, link.idx, cellToRef);
    }
  }
}

static IdxMap findMffcBounds(Builder &builder,
                             size_t root,
                             const Nodes &leaves,
                             IdxMap &newToOld) {
  IdxMap cellToRef;
  IdxMap oldToNew;
  leaveNumber = 0;

  builder.startSession();
  for (size_t i = 0; i < leaves.size(); ++i) {
    builder.markEntry(leaves[i]);
  }
  dereferenceCells(builder, root, cellToRef);
  builder.endSession();

  builder.startSession();
  oldToNew.reserve(cellToRef.size());
  getMffcBounds(builder, root, cellToRef, newToOld, oldToNew);
  builder.endSession();

  return oldToNew;
}

Fragment getMffc(Builder &builder, size_t root, const Nodes &leaves) {
  assert(!leaves.empty() && "Bounds for a fanout-free cone are empty!");
  Fragment cone;
  const size_t nLeaves = leaves.size();
  cone.subnetID = model::OBJ_NULL_ID;
  cone.entryMap.reserve(nLeaves + 1);

  if (nLeaves < 2) {
    cone.subnetID = getReconvergenceCone(builder, root,
                                         Cell::MaxArity, cone.entryMap);
    return cone;
  }

  IdxMap oldToNew = findMffcBounds(builder, root, leaves, cone.entryMap);

  Builder coneBuilder;
  coneBuilder.addInputs(oldToNew.size());
  buildFromRoot(builder, coneBuilder, root, oldToNew);
  size_t outId = coneBuilder.addOutput(Link(oldToNew.at(root))).idx;

  cone.entryMap[outId] = root;
  cone.subnetID = coneBuilder.make();

  return cone;
}

} // namespace eda::gate::optimizer
