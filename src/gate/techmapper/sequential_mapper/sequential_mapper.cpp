//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/techmapper/sequential_mapper/sequential_mapper.h"

#include <cassert>

namespace eda::gate::tech_optimizer {

SequentialMapper::SequentialMapper(CellDB *cellDB)
    : cells(std::move(cellDB)) {}

model::CellTypeID SequentialMapper::mapSequenceCell(model::CellID sequenceCellID,
                                                  Techmapper::MapperType techmapSelector) {
  auto& sequenceCell = model::Cell::get(sequenceCellID);
  assert(sequenceCell.isDff() || sequenceCell.isDffRs() || sequenceCell.isLatch());

  model::SubnetID subnetId;

  if (sequenceCell.isDff()) {
    subnetId = mapDFF(techmapSelector);
  } else if (sequenceCell.isDffRs()) {
    subnetId = mapDFFrs(techmapSelector);
  } else if (sequenceCell.isLatch()) {
    subnetId = mapLatch(techmapSelector);
  }

  auto &subnet = model::Subnet::get(subnetId);
  return subnet.getEntries()[subnet.getInNum()].cell.getTypeID();

  //return model::CellType{}; // Assuming model::SubnetID{} is a valid default or error value
}

model::SubnetID SequentialMapper::mapLatch(Techmapper::MapperType techmapSelector) {
  return chooseMappingStrategy(cells->getLatch(), techmapSelector);
}

model::SubnetID SequentialMapper::mapDFFrs(Techmapper::MapperType techmapSelector) {
  return chooseMappingStrategy(cells->getDFFrs(), techmapSelector);
}

model::SubnetID SequentialMapper::mapDFF(Techmapper::MapperType techmapSelector) {
  return chooseMappingStrategy(cells->getDFF(), techmapSelector);
}

model::SubnetID SequentialMapper::chooseMappingStrategy(const std::vector<std::pair<model::SubnetID, Subnetattr>>& seqCells,
                                                        Techmapper::MapperType techmapSelector) {
  switch (techmapSelector) {
    case Techmapper::MapperType::SIMPLE_AREA_FUNC:
      return areaOptimizedMapping(seqCells);
      /* More strategies can be added here */
    default:
      return model::SubnetID{}; // Assuming model::SubnetID{} is a valid default or error value
  }
}

model::SubnetID SequentialMapper::areaOptimizedMapping(const std::vector<std::pair<model::SubnetID, Subnetattr>>& seqCells) {
  std::pair<model::SubnetID, Subnetattr> minAreaCell =
      *std::min_element(seqCells.begin(), seqCells.end(),
                        [](const auto& lhs, const auto& rhs) {
                        return lhs.second.area < rhs.second.area;});
  return minAreaCell.first;
}

} // namespace eda::gate::tech_optimizer
