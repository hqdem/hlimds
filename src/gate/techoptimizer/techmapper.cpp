//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/celltype.h"
#include "gate/techoptimizer/cut_based_tech_mapper/cut_based_tech_mapper.h"
#include "gate/techoptimizer/cut_based_tech_mapper/strategy/simple/simplpe_area.h"
#include "gate/techoptimizer/sequential_mapper/sequential_mapper.h"
#include "gate/techoptimizer/techmapper.h"

//#include <list>
#include <map>

using CellID = eda::gate::model::CellID;

namespace eda::gate::tech_optimizer {

void Techmapper::setLiberty(const std::string &dbPath) {
  std::vector<CellTypeID> cellTypeIDs;
  LibraryCells::readLibertyFile(dbPath, cells);
  LibraryCells::makeCellTypeIDs(cells, cellTypeIDs);
  this->cellDB = new CellDB(cellTypeIDs);

  //setSequenceDB(cellDB);
}

void Techmapper::setMapper(TechmapperType techmapSelector) {
  switch(techmapSelector) {
    case TechmapperType::FUNC: // // cut-based matching
      mapper = new CutBasedTechMapper(cellDB);
      break;

    case TechmapperType::STRUCT: // DAGON matching
      //mapper = new DagonTechMapper(*cellDB);
      break;
  }
}

void Techmapper::setStrategy(TechmapperStrategyType strategySelector) {
  switch(strategySelector) {
    case TechmapperStrategyType::AREA_FLOW: // areaFlow
      //AreaFlow *strategy = new AreaFlow();
      //mapper->setStrategy(strategy);
      break;

    case TechmapperStrategyType::DELAY: // delay
      break;

    case TechmapperStrategyType::POWER: // power
      break;
    case TechmapperStrategyType::SIMPLE: // simple area
      SimplifiedStrategy *strategy = new SimplifiedStrategy();
      mapper->setStrategy(strategy);
      break;
  }
}

SubnetID Techmapper::techmap(SubnetID subnetID) {
 assert(mapper != nullptr);
 return mapper->techMap(subnetID);
}

SubnetID Techmapper::techmap(model::CellID sequenceCell) {
  return mapSequenceCell(sequenceCell);;
}
} // namespace eda::gate::tech_optimizer


