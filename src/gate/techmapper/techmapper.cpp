//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/celltype.h"
#include "gate/model/decomposer/net_decomposer.h"
#include "gate/premapper/aigmapper.h"
#include "gate/techmapper/assembly.h"
#include "gate/techmapper/comb_mapper/cut_based/area_recovery/area_recovery.h"
#include "gate/techmapper/comb_mapper/cut_based/genetic/genetic_mapper.h"
#include "gate/techmapper/comb_mapper/cut_based/power_map/power_map.h"
#include "gate/techmapper/comb_mapper/cut_based/simple_area/simple_area_mapper.h"
#include "gate/techmapper/comb_mapper/cut_based/simple_delay/simple_delay_mapper.h"
#include "gate/techmapper/library/cell.h"
#include "gate/techmapper/library/liberty_manager.h"
#include "gate/techmapper/seq_mapper/sequential_mapper.h"
#include "gate/techmapper/techmapper.h"

#include <map>
#include <unordered_map>

namespace eda::gate::techmapper {

using Cell = model::Cell;
using CellID = model::CellID;
using CellTypeID = model::CellTypeID;
using Net = model::Net;
using NetBuilder = model::NetBuilder;
using NetDecomposer = model::NetDecomposer;
using NetID = model::NetID;
using SubnetID = model::SubnetID;

Techmapper::Techmapper(const Strategy strategy,
                       const SDC& sdc,
                       const std::string &libPath) : sdc(sdc) {
  if (libPath == "") {
    assert(LibraryManager::get().isInitialized());
  } else {
    LibraryManager::get().loadLibrary(libPath);
    std::cout << "Loaded Liberty file: " << libPath << std::endl;
  }
  createCellDB();
  setMapper(strategy);
}

void Techmapper::createCellDB() {
  std::vector<CellTypeID> cells;
  std::vector<CellTypeID> ffs;
  std::vector<CellTypeID> ffrses;
  std::vector<CellTypeID> latches;

  LibraryCells::readLibertyFile(cells,
                                ffs,
                                ffrses,
                                latches);
  cellDB = new CellDB(cells,
                      ffs,
                      ffrses,
                      latches);
}

void Techmapper::setMapper(const Strategy strategy) {
  switch(strategy) {
    case Strategy::AREA:
      mapper = new SimpleAreaMapper();
      break;
    case Strategy::AREA_FLOW:
      mapper = new AreaRecovery();
      break;
    case Strategy::POWER:
      mapper = new PowerMap();
      break;
    case Strategy::GENETIC:
      mapper = new GeneticMapper();
      break;
    case Strategy::DELAY:
      mapper = new SimpleDelayMapper();
      break;
    /*case TechmapperType::STRUCT:
      mapper = new CorasickMapper();
      break;*/
  }
}

SubnetID Techmapper::techmap(const SubnetID subnetID) {
  auto AIGSubnet = premapAIGSubnet(subnetID);
  std::unordered_map<EntryIndex, BestReplacement> *bestReplacementMap =
      new std::unordered_map<EntryIndex, BestReplacement>;

  assert(mapper != nullptr);
  mapper->mapping(AIGSubnet, cellDB, sdc, bestReplacementMap);

  AssemblySubnet as;
  return as.assemblySubnet(bestReplacementMap, AIGSubnet);
}

NetID Techmapper::techmap(const NetID netID) {
  auto& decomposer = NetDecomposer::get();
  std::vector<NetDecomposer::CellMapping> mapping;
  std::vector<SubnetID> subnets;
  decomposer.decompose(netID, subnets, mapping);
  std::vector<SubnetID> mappedSubnetsID;

  for (auto const &subnet : subnets) {
    mappedSubnetsID.push_back(techmap(subnet));
  }
  NetID composedNetID = decomposer.compose(mappedSubnetsID, mapping);
  return sequenseTechMapping(composedNetID);
}

NetID Techmapper::sequenseTechMapping(const NetID netID) {
  NetBuilder netBuilder;

  Net &net = Net::get(netID);
  model::List<CellID> outs = net.getOutputs();

  std::unordered_map<CellID, CellID> buildMap;

  std::stack<CellID> stack;
  std::unordered_set<CellID> visited;

  for (const auto& out : outs) {
    stack.push(out);
    visited.insert(out);
  }

  while (!stack.empty()) {
    CellID currentCellID = stack.top();
    if (buildMap.find(currentCellID) != buildMap.end()) {
      stack.pop();
      continue;
    }

    const auto &currentCell = Cell::get(currentCellID);
    const auto &currentType = currentCell.getType();

    // TODO: add zero/one
    if (currentCell.isIn()) {
      auto cellID = makeCell(model::CellSymbol::IN);
      netBuilder.addCell(cellID);
      buildMap[currentCellID] = cellID;
      stack.pop();
      continue;
    }

    Cell::LinkList linkList;
    bool add = true;
    for (const auto &inputLink : currentCell.getLinks()) {
      if (buildMap.find(inputLink.getCellID()) != buildMap.end()) {
        linkList.push_back(model::LinkEnd{inputLink.getCellID()});
      } else {
        add = false;
      }
    }
    if (add) {
      CellID cellID;
      if (currentType.isGate() && !currentType.isCombinational()) {
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
  return netBuilder.make();
}

CellTypeID Techmapper::techmap(const CellID cellID, const Strategy strategy) {
  SequentialMapper sequentialMapper(cellDB);
  return sequentialMapper.map(cellID, strategy);
}

SubnetID Techmapper::premapAIGSubnet(SubnetID subnetID) {
  premapper::AigMapper aigMapper("aig");
  const auto transformedSub = aigMapper.transform(subnetID);
  return transformedSub;
}
} // namespace eda::gate::techmapper
