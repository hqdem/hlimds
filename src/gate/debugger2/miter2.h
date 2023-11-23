//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"
#include "util/logging.h"

using namespace eda::gate::model;

namespace eda::gate::debugger2 {

using Subnet = eda::gate::model::Subnet;
using Cell = Subnet::Cell;
using Link = Subnet::Link;
using LinkList = Subnet::LinkList;
using CellID = model::CellID;
using SubnetBuilder = model::SubnetBuilder;
using CellToCell = std::unordered_map<size_t, size_t>;

struct MiterHints {
  CellToCell sourceBinding;
  CellToCell targetBinding;
};

/**
 *  \brief Constructs a miter for the specified nets.
 *  @param hints Gate-to-gate mapping between nets.
 *  @return The miter, if it is constructable, the first net otherwise.
 */
Subnet miter2(const Subnet &net1, const Subnet &net2, MiterHints &hints);

// Checks if it is possible to construct a miter with given parameters.
bool areMiterable(const Subnet &net1, const Subnet &net2, MiterHints &hints);

// Adds nets' non-input cells to the builder and stores correspondence.
// OUT cells are added as BUF.
void buildCells(const Subnet &net,
                SubnetBuilder &builder,
                CellToCell &correspondence);
/**
 *  \brief Fills hints structure for two nets based on a map.
 *  @param net The net, which the maps' keys were based on.
 *  @param map Cell-to-Cell mapping between nets.
 *  @return Filled hints structure.
 */
MiterHints makeHints(const Subnet &net, CellToCell &map);

} // namespace eda::gate::debugger2
