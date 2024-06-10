//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/library/liberty_manager.h"
#include "gate/model/celltype.h"
#include "gate/model/decomposer/net_decomposer.h"
#include "gate/techmapper/assembly.h"
#include "gate/techmapper/comb_mapper/func_mapper/area_recovery/area_recovery.h"
#include "gate/techmapper/comb_mapper/func_mapper/power_map/power_map.h"
#include "gate/techmapper/comb_mapper/func_mapper/simple_area/simple_area_mapper.h"
#include "gate/techmapper/comb_mapper/func_mapper/simple_delay/simple_delay_mapper.h"
#include "gate/techmapper/seq_mapper/sequential_mapper.h"
#include "gate/techmapper/techmapper.h"

#include <map>
#include <unordered_map>

namespace eda::gate::techmapper {

using Cell = model::Cell;
using CellTypeID = model::CellTypeID;
using LibertyManager = library::LibertyManager;
using Net = model::Net;
using NetBuilder = model::NetBuilder;
using NetDecomposer = model::NetDecomposer;
using NetID = model::NetID;
using SDCManager = library::SDCManager;
using SubnetID = model::SubnetID;

optimizer::SubnetBuilderPtr Techmapper::make(const SubnetID subnetID) const {
  SubnetBuilder builder;
  techmap(subnetID, builder);
  auto builderPtr = std::make_shared<SubnetBuilder>(builder);
  return builderPtr;
}

void Techmapper::setSDC(const std::filesystem::path &sdcPath) {
  // TODO
  if (sdcPath == "") {
    std::cout << "Warning: SDC path is empty." << std::endl;
  } else {
    SDCManager::get().loadSDC(sdcPath);
    std::cout << "Loaded SDC file: " << sdcPath << std::endl;
  }

  sdc = &SDCManager::get().getSDC();
}

void Techmapper::setLibrary(const std::filesystem::path &libPath) {
  if (libPath == "") {
    assert(LibertyManager::get().isInitialized());
  } else {
    LibertyManager::get().loadLibrary(libPath);
    std::cout << "Loaded Liberty file: " << libPath << std::endl;
  }

  if (cellDB != nullptr) {
    delete cellDB;
  }
  cellDB = new SCLibrary();
}

void Techmapper::setStrategy(const Strategy strategy) {
  if (mapper != nullptr) {
    delete mapper;
  }
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
    case Strategy::DELAY:
      mapper = new SimpleDelayMapper();
      break;
    /*case TechmapperType::STRUCT:
      mapper = new CorasickMapper();
      break;*/
  }
}

void Techmapper::techmap(const SubnetID subnetID,
                         SubnetBuilder &builder) const {
  Mapping *mapping = new Mapping;

  assert(mapper != nullptr);
  mapper->map(subnetID, *cellDB, *sdc, *mapping);

  AssemblySubnet as;
  as.assemble(subnetID, *mapping, builder);
}

NetID Techmapper::techmap(const NetID netID) {
  auto& decomposer = NetDecomposer::get();
  std::vector<NetDecomposer::CellMapping> mapping;
  std::vector<SubnetID> subnets;
  decomposer.decompose(netID, subnets, mapping);
  std::vector<SubnetID> mappedSubnetsID;

  for (auto const &subnet : subnets) {
    // TODO: premapping to AIG
    SubnetBuilder builder;
    techmap(subnet, builder);
    mappedSubnetsID.push_back(builder.make());
  }
  NetID composedNetID = decomposer.compose(mappedSubnetsID, mapping);
  return seqTechmap(composedNetID);
}

NetID Techmapper::seqTechmap(const NetID netID) {
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

} // namespace eda::gate::techmapper
