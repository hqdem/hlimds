//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/techoptimizer/cut_based_tech_mapper/cut_based_tech_mapper.h"
#include "gate/techoptimizer/library/cell.h"
#include "gate/techoptimizer/library/cellDB.h"
#include "gate/techoptimizer/sequential_mapper/sequential_mapper.h"
#include "gate/techoptimizer/techmapper.h"

#include <list>
#include <map>

namespace eda::gate::tech_optimizer {

using CellID = eda::gate::model::CellID;

CellDB cellDB;
BaseMapper *mapper = nullptr;

void setLiberty(const std::string &dbPath) {
  LibraryCells libraryCells(dbPath);

  CellDB *cellDB_ptr = &cellDB;
  *cellDB_ptr = libraryCells.initializeLiberty();

  setSequenceDB(cellDB);
}

void setMapper(TechmapType techmapSelector) {
  switch(techmapSelector) {
    case TechmapType::FUNC: // // cut-based matching
      mapper = new CutBasedTechMapper(cellDB);
      break;

    case TechmapType::STRUCT: // DAGON matching
      //mapper = new DagonTechMapper(cellDB);
      break;
  }
}

void setStrategy(StrategyType strategySelector) {
  switch(strategySelector) {
    case StrategyType::AREA_FLOW: // areaFlow
      //AreaFlow *strategy = new AreaFlow();
      //mapper->setStrategy(strategy);
      break;

    case StrategyType::DELAY: // delay
      break;

    case StrategyType::POWER: // power
      break;
  }
}

SubnetID techmap(SubnetID subnetID) {
 assert(mapper != nullptr);
 return mapper->techMap(subnetID);
}

SubnetID techmap(model::Subnet::Cell sequenceCell) {
  return mapSequenceCell(sequenceCell);;
}
} // namespace eda::gate::tech_optimizer


