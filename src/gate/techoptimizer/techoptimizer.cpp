//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/celltype.h"
#include "gate/techoptimizer/assembly.h"
#include "gate/techoptimizer/mapper/cut_base/power_map/power_map.h"
#include "gate/techoptimizer/mapper/cut_base/simple_area/simple_area_mapper.h"
#include "gate/techoptimizer/sequential_mapper/sequential_mapper.h"
#include "gate/techoptimizer/techoptimizer.h"
#include "gate/transformer/aigmapper.h"
#include "gate/model2/decomposer/net_decomposer.h"

//#include <list>
#include <map>
#include <unordered_map>

using CellID = eda::gate::model::CellID;

namespace eda::gate::tech_optimizer {

Techmapper::Techmapper(const std::string &dbPath,
                            MapperType techmapSelector,
                            SDC& sdc) {
  this->sdc = sdc;
  setLiberty(dbPath);
  setMapper(techmapSelector);
}

void Techmapper::setLiberty(const std::string &dbPath) {
  std::vector<CellTypeID> cellTypeIDs;
  std::vector<CellTypeID> ffCellTypeIDs;
  std::vector<CellTypeID> ffrsCellTypeIDs;
  std::vector<CellTypeID> LatchCellTypeIDs;
  LibraryCells::readLibertyFile(dbPath,
                                cellTypeIDs,
                                ffCellTypeIDs,
                                ffrsCellTypeIDs,
                                LatchCellTypeIDs);
  //LibraryCells::makeCellTypeIDs(cells, cellTypeIDs);
  this->cellDB = new CellDB(cellTypeIDs,
                            ffCellTypeIDs,
                            ffrsCellTypeIDs,
                            LatchCellTypeIDs);

}

void Techmapper::setMapper(MapperType techmapSelector) {
  switch(techmapSelector) {
    case MapperType::SIMPLE_AREA_FUNC:
      mapper = new SimpleAreaMapper();
      break;
    case MapperType::AREA_FLOW:
      break;
    case MapperType::DELAY:
      break;
    case MapperType::POWER:
      mapper = new PowerMap();
      break;

    /*case TechmapperType::STRUCT: // DAGON matching
      //mapper = new DagonTechMapper(*cellDB);
      break;*/
  }
}

SubnetID Techmapper::techmap(SubnetID subnetID) {
  auto AIGSubnet = premapAIGSubnet(subnetID);
  std::unordered_map<EntryIndex, BestReplacement> *bestReplacementMap =
      new std::unordered_map<EntryIndex, BestReplacement>;

 assert(mapper != nullptr);

  mapper->mapping(AIGSubnet, cellDB, sdc, bestReplacementMap);

  std::cout << "start to create new mapped Subnet" << std::endl;
  AssemblySubnet as;
 return as.assemblySubnet(bestReplacementMap, AIGSubnet);
}

NetID Techmapper::techmap(NetID netID) {
  auto& decomposer = eda::gate::model::NetDecomposer::get();
  std::vector<eda::gate::model::NetDecomposer::CellMapping> mapping;
  std::vector<SubnetID> subnets = decomposer.decompose(netID, mapping);
  std::vector<SubnetID> mappedSubnetsID;

  for (auto const subnet : subnets) {
    mappedSubnetsID.push_back(techmap(subnet));
  }
  eda::gate::model::NetID composedNetID = decomposer.compose(mappedSubnetsID, mapping);

  return composedNetID;
}

/*NetID Techmapper::sequenseTechMapping(NetID netID) {
  model::NetBuilder netBuilder;

  model::Net::get(netID).

  return netBuilder.make();
}*/

SubnetID Techmapper::techmap(model::CellID sequenceCell,
                             MapperType techmapSelector) {
  SequentialMapper sequentialMapper(cellDB);
  return sequentialMapper.mapSequenceCell(sequenceCell, techmapSelector);
}

SubnetID Techmapper::premapAIGSubnet(SubnetID subnetID) {
  auto startAIG = std::chrono::high_resolution_clock::now();
  std::cout << "Convert to AIG" << std::endl;
  transformer::AigMapper aigMapper;
  const auto transformedSub  = aigMapper.transform(subnetID);
  auto endAIG = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> AIGTime = endAIG - startAIG;
  std::cout << "Функция AIG выполнялась " << AIGTime.count() << " секунд.\n";
  return transformedSub;
}
} // namespace eda::gate::tech_optimizer


