//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/celltype.h"
#include "gate/techoptimizer/sequential_mapper/sequential_mapper.h"
#include "gate/techoptimizer/techoptimizer.h"
#include "gate/techoptimizer/mapper/cut_base/simple_area/simple_area_mapper.h"
#include "gate/transformer/aigmapper.h"
#include "gate/techoptimizer/assembly.h"

//#include <list>
#include <map>

using CellID = eda::gate::model::CellID;

namespace eda::gate::tech_optimizer {

Techmapper::Techmapper(const std::string &dbPath,
                            MapperType techmapSelector) {
  setLiberty(dbPath);
  setMapper(techmapSelector);
}

void Techmapper::setLiberty(const std::string &dbPath) {
  std::vector<Cell*> cells;
  std::vector<CellTypeID> cellTypeIDs;
  std::vector<CellTypeID> ffCellTypeIDs;
  std::vector<CellTypeID> ffrsCellTypeIDs;
  std::vector<CellTypeID> LatchCellTypeIDs;
  LibraryCells::readLibertyFile(dbPath, cells);
  LibraryCells::makeCellTypeIDs(cells, cellTypeIDs);
  this->cellDB = new CellDB(cellTypeIDs);

  //setSequenceDB(cellDB);
}

void Techmapper::setMapper(MapperType techmapSelector) {
  switch(techmapSelector) {
    case MapperType::SIMPLE_AREA_FUNC: // // cut-based matching
      mapper = new SimpleAreaMapper();
      break;

    /*case TechmapperType::STRUCT: // DAGON matching
      //mapper = new DagonTechMapper(*cellDB);
      break;*/
  }
}

SubnetID Techmapper::techmap(SubnetID subnetID) {
  std::map<EntryIndex, BestReplacement> *bestReplacementMap =
      new std::map<EntryIndex, BestReplacement>;

 assert(mapper != nullptr);

  mapper->mapping(subnetID, cellDB, bestReplacementMap);

  AssemblySubnet as;
 return as.assemblySubnet(bestReplacementMap, subnetID);
}

SubnetID Techmapper::techmap(model::CellID sequenceCell) {
  return mapSequenceCell(sequenceCell);;
}

SubnetID Techmapper::premapAIGSubnet(SubnetID subnetID) {
  transformer::AigMapper aigMapper;
  const auto transformedSub  = aigMapper.transform(subnetID);
  return transformedSub;
}
} // namespace eda::gate::tech_optimizer


