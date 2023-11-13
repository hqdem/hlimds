//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/techoptimizer/sequential_mapper/sequential_mapper.h"

namespace eda::gate::tech_optimizer {
  void mapDFF( LinkList linkList) {
    CellTypeID cellID = eda::gate::model::makeCellType(
        "DFF", eda::gate::model::CellSymbol::CELL,
        props, static_cast<uint16_t>(cell->getInputPinsNumber()), 
        static_cast<uint16_t>(1));
    auto cellID = makeCell(CellSymbol::DFF, linkList)



    LibraryCells libraryCells(dbPath);
      functDB.linkDB("rwtest.db");
      functDB.openDB();

      libraryCells.initializeLibraryRwDatabase(&functDB, cellTypeMap);
  }
} // namespace eda::gate::tech_optimizer