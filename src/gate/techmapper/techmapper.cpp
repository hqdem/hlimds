//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/celltype.h"
#include "gate/model2/decomposer/net_decomposer.h"
#include "gate/premapper2/aigmapper.h"
#include "gate/techmapper/assembly.h"

#include "gate/techmapper/comb_mapper/cut_based/area_recovery/area_recovery.h"
#include "gate/techmapper/comb_mapper/cut_based/genetic/genetic_mapper.h"
#include "gate/techmapper/comb_mapper/cut_based/power_map/power_map.h"
#include "gate/techmapper/comb_mapper/cut_based/simple_area/simple_area_mapper.h"
#include "gate/techmapper/comb_mapper/cut_based/simple_delay/simple_delay_mapper.h"
#include "gate/techmapper/library/libertyManager.h"
#include "gate/techmapper/seq_mapper/sequential_mapper.h"
#include "gate/techmapper/techmapper.h"

#include <map>
#include <unordered_map>

using CellID = eda::gate::model::CellID;

namespace eda::gate::techmapper {

Techmapper::Techmapper(const std::string &dbPath,
                            MapperType techmapSelector,
                            SDC& sdc) {
  LibraryManager::get().loadLibrary(dbPath);
  this->sdc = sdc;
  setLiberty();
  setMapper(techmapSelector);
}

Techmapper::Techmapper(MapperType techmapSelector,
                       SDC& sdc) {
  this->sdc = sdc;
  setLiberty();
  setMapper(techmapSelector);
}

void Techmapper::setLiberty() {
  std::vector<CellTypeID> cellTypeIDs;
  std::vector<CellTypeID> ffCellTypeIDs;
  std::vector<CellTypeID> ffrsCellTypeIDs;
  std::vector<CellTypeID> LatchCellTypeIDs;
  LibraryCells::readLibertyFile(cellTypeIDs,
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
      mapper = new AreaRecovery();
      break;
    case MapperType::DELAY:
      break;
    case MapperType::POWER:
      mapper = new PowerMap();
      break;
    case MapperType::GENETIC:
      mapper = new GeneticMapper();
      break;
    case MapperType::SIMPLE_DELAY_FUNC:
      mapper = new SimpleDelayMapper();
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

  AssemblySubnet as;
  return as.assemblySubnet(bestReplacementMap, AIGSubnet);
}

NetID Techmapper::techmap(NetID netID) {
  auto& decomposer = eda::gate::model::NetDecomposer::get();
  std::vector<eda::gate::model::NetDecomposer::CellMapping> mapping;
  std::vector<SubnetID> subnets;
  decomposer.decompose(netID, subnets, mapping);
  std::vector<SubnetID> mappedSubnetsID;

  for (auto const &subnet : subnets) {
    mappedSubnetsID.push_back(techmap(subnet));
  }
  eda::gate::model::NetID composedNetID = decomposer.compose(mappedSubnetsID, mapping);

  return sequenseTechMapping(composedNetID);
}

NetID Techmapper::sequenseTechMapping(NetID netID) {
  model::NetBuilder netBuilder;

  model::Net &net = model::Net::get(netID);
  model::List<model::CellID> outs = net.getOutputs();

  std::unordered_map<model::CellID, model::CellID> buildMap;

  std::stack<model::CellID> stack;
  std::unordered_set<model::CellID> visited;

  for (const auto& out : outs) {
    stack.push(out);
    visited.insert(out);
  }

  while (!stack.empty()) {
    model::CellID currentCellID = stack.top();
    if (buildMap.find(currentCellID) != buildMap.end()) {
      stack.pop();
      continue;
    }

    auto &currentCell = model::Cell::get(currentCellID);
    // TODO: add zero/one
    if (currentCell.isIn()) {
      auto cellID = makeCell(model::CellSymbol::IN);
      netBuilder.addCell(cellID);
      buildMap[currentCellID] = cellID;
      stack.pop();
      continue;
    } else {
      model::Cell::LinkList linkList;
      bool add = true;
      for (const auto &inputLink : currentCell.getLinks()) {
        if (buildMap.find(inputLink.getCellID()) != buildMap.end()) {
          linkList.push_back(model::LinkEnd{inputLink.getCellID()});
        } else {
          add = false;
        }
      }
      if (add) {
        model::CellID cellID;
        if (currentCell.isDff() || currentCell.isDffRs() || currentCell.isLatch()) {
          // Change cellType
          cellID = makeCell(techmap(currentCellID), linkList);
        } else {
          cellID = makeCell(currentCell.getTypeID(), linkList);
        }
        buildMap[currentCellID] = cellID;
        netBuilder.addCell(cellID);
        stack.pop();
      }

      for (const auto& link : currentCell.getLinks()) {
        stack.push(link.getCellID());
      }
    }
  }

  return netBuilder.make();
}

model::CellTypeID Techmapper::techmap(model::CellID sequenceCell,
                             MapperType techmapSelector) {
  SequentialMapper sequentialMapper(cellDB);
  return sequentialMapper.mapSequenceCell(sequenceCell, techmapSelector);
}

SubnetID Techmapper::premapAIGSubnet(SubnetID subnetID) {
  premapper2::AigMapper aigMapper;
  const auto transformedSub  = aigMapper.transform(subnetID);
  return transformedSub;
}
} // namespace eda::gate::techmapper
