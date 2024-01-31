//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"
#include "util/logging.h"

namespace eda::gate::debugger2 {

using Cell = model::Subnet::Cell;
using CellSymbol = model::CellSymbol;
using CellToCell = std::unordered_map<size_t, size_t>;
using Link = model::Subnet::Link;
using LinkList = model::Subnet::LinkList;
using Subnet = eda::gate::model::Subnet;
using SubnetBuilder = model::SubnetBuilder;

/// Gate-to-gate mapping between corresponding inputs and outputs of two nets.
struct MiterHints {
  /// Gate-to-gate mapping between inputs of two nets.
  CellToCell sourceBinding;
  /// Gate-to-gate mapping between outputs of two nets.
  CellToCell targetBinding;
};

/**
 * \brief Constructs a miter for the specified nets.
 * @param net1 First net.
 * @param net2 Second net.
 * @param gmap Gate-to-gate mapping between corresponding PI/PO of two nets.
 * @return The miter , if it is constructable, the first net otherwise.
 */
const Subnet &miter2(const Subnet &net1,
                     const Subnet &net2,
                     const CellToCell &gmap);

/**
 * \brief Fills hints structure for two nets based on a map.
 * @param net The net, which the maps' keys were based on.
 * @param map Cell-to-Cell mapping between nets.
 * @return Filled hints structure.
 */
MiterHints makeHints(const Subnet &net, const CellToCell &map);

} // namespace eda::gate::debugger2
