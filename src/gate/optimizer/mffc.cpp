//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/mffc.h"

namespace eda::gate::optimizer {

using Builder    = model::SubnetBuilder;
using Cell       = model::Subnet::Cell;
using EntryID    = model::EntryID;
using Link       = model::Subnet::Link;
using LinkList   = model::Subnet::LinkList;
using Nodes      = model::EntryIDList;
using SubnetView = model::SubnetView;

static void getMffcBounds(Builder &builder,
                          EntryID idx,
                          size_t &counter,
                          uint32_t boundsID,
                          Nodes &bounds) {

  for (const auto &link : builder.getLinks(idx)) {
    const auto index = link.idx;
    Cell &cell = builder.getCell(index);
    if (builder.isMarked(index) || cell.isZero() || cell.isOne()) {
      cell.incRefCount();
      counter--;
      continue;
    }

    if ((builder.getSessionID(index) == boundsID) || cell.isIn()) {
      builder.mark(index);
      bounds.push_back(index);
      cell.incRefCount();
      counter--;
      continue;
    }
    builder.mark(index);

    if (!cell.refcount) {
      getMffcBounds(builder, index, counter, boundsID, bounds);
      cell.incRefCount();
      counter--;
      continue;
    }

    cell.incRefCount();
    counter--;

    bounds.push_back(index);
  }
}

static void findMffcBounds(Builder &builder,
                           EntryID rootID,
                           size_t counter,
                           uint32_t boundsID,
                           Nodes &bounds) {

  // Reference cells back and get inputs(bounds) of MFFC.
  builder.startSession();
  getMffcBounds(builder, rootID, counter, boundsID, bounds);
  builder.endSession();
  assert(!counter && "Unequal number of reference and dereference operations");
}

//===----------------------------------------------------------------------===//
// Bound -- CUT
//===----------------------------------------------------------------------===//

static void dereferenceCells(Builder &builder, EntryID idx, size_t &counter) {
  for (const auto &link : builder.getLinks(idx)) {
    const auto index = link.idx;
    Cell &cell = builder.getCell(index);
    cell.decRefCount();
    counter++;

    if (!cell.refcount && !builder.isMarked(index)) {
      dereferenceCells(builder, index, counter);
    }
  }
}

static uint32_t dereferenceCells(Builder &builder,
                                 EntryID rootID,
                                 size_t &counter,
                                 const Nodes &cut) {

  builder.startSession();
  const auto boundsID = builder.getSessionID();
  for (const auto &idx : cut) {
    builder.mark(idx);
  }
  dereferenceCells(builder, rootID, counter);
  builder.endSession();
  return boundsID;
}

SubnetView getMffc(Builder &builder, EntryID rootID, const Nodes &cut) {
  const auto nLeaves = cut.size();
  if ((nLeaves == 1) && (rootID == cut[0])) {
    return SubnetView{builder, {Nodes{rootID}, Nodes{rootID}}};
  }

  size_t counter = 0;
  const auto boundsID = dereferenceCells(builder, rootID, counter, cut);

  Nodes bounds;
  bounds.reserve(cut.size());
  findMffcBounds(builder, rootID, counter, boundsID, bounds);
  Nodes output{rootID};

  return SubnetView{builder, {std::move(bounds), std::move(output)}};
}

SubnetView getMffc(Builder &builder, const SubnetView &view) {
  const Nodes &roots = view.getOutputs();
  assert((roots.size() == 1) && "Multiple outputs are not supported");
  return getMffc(builder, roots[0], view.getInputs());
}

SubnetView getMffc(Builder &builder, EntryID rootID) {
  Nodes emptyBounds;
  return getMffc(builder, rootID, emptyBounds);
}

//===----------------------------------------------------------------------===//
// Bound -- DEPTH
//===----------------------------------------------------------------------===//

static void dereferenceCells(Builder &builder,
                             EntryID idx,
                             size_t &counter,
                             uint32_t maxDepth,
                             uint32_t depth) {

  for (const auto &link : builder.getLinks(idx)) {
    const auto index = link.idx;
    Cell &cell = builder.getCell(index);
    cell.decRefCount();
    counter++;

    if (depth >= maxDepth) {
      builder.mark(index);
    }

    if (!cell.refcount && !builder.isMarked(index)) {
      dereferenceCells(builder, index, counter, maxDepth, depth + 1);
    }
  }
}

static uint32_t dereferenceCells(Builder &builder,
                                 EntryID rootID,
                                 size_t &counter,
                                 uint32_t maxDepth) {

  builder.startSession();
  const auto boundsID = builder.getSessionID();
  dereferenceCells(builder, rootID, counter, maxDepth, 1);
  builder.endSession();
  return boundsID;
}

SubnetView getMffc(Builder &builder, EntryID rootID, uint32_t maxDepth) {
  if (!maxDepth) {
    return SubnetView{builder, {Nodes{rootID}, Nodes{rootID}}};
  }

  size_t counter = 0;
  const auto boundsID = dereferenceCells(builder, rootID, counter, maxDepth);

  Nodes bounds;
  bounds.reserve(1u << (maxDepth + 1));
  findMffcBounds(builder, rootID, counter, boundsID, bounds);
  Nodes output{rootID};

  return SubnetView{builder, {std::move(bounds), std::move(output)}};
}

} // namespace eda::gate::optimizer
