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

CellDB cells;

model::SubnetID mapDFF();
model::SubnetID mapDFFrs();
model::SubnetID mapLATCH();

void setSequenceDB(CellDB &cellDB) {
  cells = cellDB;
}

model::SubnetID mapSequenceCell(model::CellID sequenceCellID) {
  auto &sequenceCell = model::Cell::get(sequenceCellID);

  assert(sequenceCell.isDff() || sequenceCell.isDffRs() || sequenceCell.isLatch());

  if (sequenceCell.isDff()) {
    return mapDFF();
  } else if (sequenceCell.isDffRs()) {
    return mapDFFrs();
  } else if (sequenceCell.isLatch()) {
    return mapLATCH();
  } else {
    return 1;
  }
}

model::SubnetID mapLATCH() {
  return cells.getLatch()[0];
}

model::SubnetID mapDFFrs() {
  return cells.getDFFrs()[0];
}

model::SubnetID mapDFF() {
  return cells.getDFF()[0];
}
} // namespace eda::gate::tech_optimizer