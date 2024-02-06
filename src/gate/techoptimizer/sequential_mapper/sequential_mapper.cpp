//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/techoptimizer/sequential_mapper/sequential_mapper.h"
#include "gate/techoptimizer/library/cellDB.h"

using CellSymbol = eda::gate::model::CellSymbol;

namespace eda::gate::tech_optimizer {

CellDB *cells;

model::SubnetID mapDFF(Techmapper::MapperType techmapSelector);
model::SubnetID mapDFFrs(Techmapper::MapperType techmapSelector);
model::SubnetID mapLATCH(Techmapper::MapperType techmapSelector);
model::SubnetID areaStrategy(const std::vector<std::pair<model::SubnetID, Subnetattr>> &seqCells);
model::SubnetID choiceOfStrategy(const std::vector<std::pair<model::SubnetID, Subnetattr>> &seqCells,
                                 Techmapper::MapperType techmapSelector);

void setSequenceDB(CellDB *cellDB) {
  cells = cellDB;
}

model::SubnetID mapSequenceCell(model::CellID sequenceCellID,
                                Techmapper::MapperType techmapSelector) {
  auto &sequenceCell = model::Cell::get(sequenceCellID);

  assert(sequenceCell.isDff() || sequenceCell.isDffRs() || sequenceCell.isLatch());

  if (sequenceCell.isDff()) {
    return mapDFF(techmapSelector);
  } else if (sequenceCell.isDffRs()) {
    return mapDFFrs(techmapSelector);
  } else if (sequenceCell.isLatch()) {
    return mapLATCH(techmapSelector);
  } else {
    return 1;
  }
}

model::SubnetID mapLATCH(Techmapper::MapperType techmapSelector) {
  return choiceOfStrategy(cells->getLatch(), techmapSelector);;
}

model::SubnetID mapDFFrs(Techmapper::MapperType techmapSelector) {
  return choiceOfStrategy(cells->getDFFrs(), techmapSelector);
}

model::SubnetID mapDFF(Techmapper::MapperType techmapSelector) {
  return choiceOfStrategy(cells->getDFF(), techmapSelector);
}

model::SubnetID choiceOfStrategy(const std::vector<std::pair<model::SubnetID, Subnetattr>> &seqCells,
                                 Techmapper::MapperType techmapSelector) {
  switch(techmapSelector) {
    case Techmapper::MapperType::SIMPLE_AREA_FUNC:
      return areaStrategy(seqCells);
      break;

    /*case TechmapperType::STRUCT: // DAGON matching
      //mapper = new DagonTechMapper(*cellDB);
      break;*/
  }
}

model::SubnetID areaStrategy(const std::vector<std::pair<model::SubnetID, Subnetattr>> &seqCells) {
  std::pair<model::SubnetID, Subnetattr> idMinArea;
  for (const auto& cell : seqCells) {
    if (cell.second.area < idMinArea.second.area) {
      idMinArea = cell;
    }
  }
  return idMinArea.first;
}
} // namespace eda::gate::tech_optimizer

