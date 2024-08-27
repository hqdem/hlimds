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
using Link       = model::Subnet::Link;
using LinkList   = model::Subnet::LinkList;
using Nodes      = std::vector<model::EntryID>;
using SubnetView = model::SubnetView;

static void getMffcBounds(Builder &builder,
                          model::EntryID idx,
                          size_t &counter,
                          size_t boundsID,
                          Nodes &bounds) {

  for (const auto &link : builder.getLinks(idx)) {
    Cell &cell = builder.getCell(link.idx);
    if (builder.isMarked(link.idx) || cell.isZero() || cell.isOne()) {
      cell.incRefCount();
      counter--;
      continue;
    }

    if ((builder.getSessionID(link.idx) == boundsID) || cell.isIn()) {
      builder.mark(link.idx);
      bounds.push_back(link.idx);
      cell.incRefCount();
      counter--;
      continue;
    }
    builder.mark(link.idx);

    if (!cell.refcount) {
      getMffcBounds(builder, link.idx, counter, boundsID, bounds);
      cell.incRefCount();
      counter--;
      continue;
    }

    cell.incRefCount();
    counter--;

    bounds.push_back(link.idx);
  }
}

static void findMffcBounds(Builder &builder,
                           model::EntryID rootID,
                           size_t counter,
                           size_t boundsID,
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

static void dereferenceCells(Builder &builder,
                             model::EntryID idx,
                             size_t &counter) {
  for (const auto &link : builder.getLinks(idx)) {
    Cell &cell = builder.getCell(link.idx);
    cell.decRefCount();
    counter++;

    if (!cell.refcount && !builder.isMarked(link.idx)) {
      dereferenceCells(builder, link.idx, counter);
    }
  }
}

static size_t dereferenceCells(Builder &builder,
                               model::EntryID rootID,
                               size_t &counter,
                               const Nodes &cut) {

  builder.startSession();
  auto boundsID = builder.getSessionID();
  for (const auto &idx : cut) {
    builder.mark(idx);
  }
  dereferenceCells(builder, rootID, counter);
  builder.endSession();
  return boundsID;
}

SubnetView getMffc(Builder &builder, model::EntryID rootID, const Nodes &cut) {
  const auto nLeaves = cut.size();
  if ((nLeaves == 1) && (rootID == cut[0])) {
    return SubnetView{builder, {Nodes{rootID}, Nodes{rootID}}};
  }

  size_t counter = 0;
  const size_t boundsID = dereferenceCells(builder, rootID, counter, cut);

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

SubnetView getMffc(Builder &builder, model::EntryID rootID) {
  Nodes emptyBounds;
  return getMffc(builder, rootID, emptyBounds);
}

//===----------------------------------------------------------------------===//
// Bound -- DEPTH
//===----------------------------------------------------------------------===//

static void dereferenceCells(Builder &builder,
                             model::EntryID idx,
                             size_t &counter,
                             size_t maxDepth,
                             size_t depth) {

  for (const auto &link : builder.getLinks(idx)) {
    Cell &cell = builder.getCell(link.idx);
    cell.decRefCount();
    counter++;

    if (depth >= maxDepth) {
      builder.mark(link.idx);
    }

    if (!cell.refcount && !builder.isMarked(link.idx)) {
      dereferenceCells(builder, link.idx, counter, maxDepth, depth + 1);
    }
  }
}

static size_t dereferenceCells(Builder &builder,
                               model::EntryID rootID,
                               size_t &counter,
                               size_t maxDepth) {

  builder.startSession();
  size_t boundsID = builder.getSessionID();
  dereferenceCells(builder, rootID, counter, maxDepth, 1);
  builder.endSession();
  return boundsID;
}

SubnetView getMffc(Builder &builder, model::EntryID rootID, size_t maxDepth) {
  if (!maxDepth) {
    return SubnetView{builder, {Nodes{rootID}, Nodes{rootID}}};
  }

  size_t counter = 0;
  const size_t boundsID = dereferenceCells(builder, rootID, counter, maxDepth);

  Nodes bounds;
  bounds.reserve(1u << (maxDepth + 1));
  findMffcBounds(builder, rootID, counter, boundsID, bounds);
  Nodes output{rootID};

  return SubnetView{builder, {std::move(bounds), std::move(output)}};
}

} // namespace eda::gate::optimizer
