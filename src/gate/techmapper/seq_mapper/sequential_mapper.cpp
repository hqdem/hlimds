//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/techmapper/seq_mapper/sequential_mapper.h"

#include <cassert>

namespace eda::gate::techmapper {

using Cell = model::Cell;
using CellID = model::CellID;
using CellTypeID = model::CellTypeID;
using Strategy = Techmapper::Strategy;

CellTypeID SequentialMapper::map(const CellID cellID,
                                 const Strategy strategy) {
  const auto &cell = Cell::get(cellID);
  const auto &type = cell.getType();
  
  assert(type.isGate() && !type.isCombinational());

  SubnetID subnetID;
  if (type.isDff()) {
    //subnetID = findSubnetID(cellDB->getDFFs(), strategy); // FIXME:
  } else if (type.isDffRs()) {
    //subnetID = findSubnetID(cellDB->getDFFrses(), strategy); // FIXME:
  } else if (type.isDLatch()) {
    //subnetID = findSubnetID(cellDB->getLatches(), strategy); // FIXME:
  } else {
    //assert(false && "Unsupported cell type");
  }

  auto &subnet = model::Subnet::get(subnetID);
  return subnet.getEntries()[subnet.getInNum()].cell.getTypeID();
}

SubnetID SequentialMapper::findSubnetID(
    std::vector<std::pair<SubnetID, SCAttrs>> &cells,
    const Strategy strategy) {
  if (strategy == Strategy::AREA) {
    // Selecting cell with the min area.
    const auto cell =
      std::min_element(cells.begin(), cells.end(),
        [](const auto &lhs, const auto &rhs) {
        return lhs.second.area < rhs.second.area;});
    return cell->first;
      /* More strategies can be added here */
  }

  assert(false && "Unsupported strategy");
  return model::OBJ_NULL_ID;
}

} // namespace eda::gate::techmapper
