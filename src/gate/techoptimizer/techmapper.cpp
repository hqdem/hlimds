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
#include "gate/techoptimizer/cut_based_tech_mapper/strategy/simple/simplpe_area.h"

#include <list>
#include <map>

using CellID = eda::gate::model::CellID;

namespace eda::gate::tech_optimizer {

CellDB cellDB;
BaseMapper *mapper = nullptr;

void Techmaper::setLiberty(const std::string &dbPath) {
  LibraryCells libraryCells(dbPath);

  CellDB *cellDB_ptr = &cellDB;
  *cellDB_ptr = libraryCells.initializeLiberty();

  setSequenceDB(cellDB);
}

void Techmaper::setMapper(TechmaperType techmapSelector) {
  switch(techmapSelector) {
    case TechmaperType::FUNC: // // cut-based matching
      mapper = new CutBasedTechMapper(cellDB);
      break;

    case TechmaperType::STRUCT: // DAGON matching
      //mapper = new DagonTechMapper(cellDB);
      break;
  }
}

void Techmaper::setStrategy(TechmaperStrategyType strategySelector) {
  switch(strategySelector) {
    case TechmaperStrategyType::AREA_FLOW: // areaFlow
      //AreaFlow *strategy = new AreaFlow();
      //mapper->setStrategy(strategy);
      break;

    case TechmaperStrategyType::DELAY: // delay
      break;

    case TechmaperStrategyType::POWER: // power
      break;
    case TechmaperStrategyType::SIMPLE: // simple area
      SimplifiedStrategy *strategy = new SimplifiedStrategy();
      mapper->setStrategy(strategy);
      break;
  }
}

SubnetID Techmaper::techmap(SubnetID subnetID) {
 assert(mapper != nullptr);
 return mapper->techMap(subnetID);
}

SubnetID Techmaper::techmap(model::CellID sequenceCell) {
  return mapSequenceCell(sequenceCell);;
}
} // namespace eda::gate::tech_optimizer


