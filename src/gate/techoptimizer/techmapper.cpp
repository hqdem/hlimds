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

void addInputToNetBuilder(model::NetBuilder &netBuilder,
    model::List<CellID> inputs, std::map<CellID, CellID> &cellMap);
std::list<CellID> getNetCellID(std::list<CellID> &subnetInputs, std::map<CellID,
    CellID>  &cellMap);
NetID mapSequenceNet(NetID net, CutBasedTechMapper *mapper);
NetID mapCombNet(NetID netID, CutBasedTechMapper *mapper);
SubnetID getSubnet(NetID netID, std::list<CellID> subnetInputs, CellID subnetOutput);

CellDB cellDB;

void read_db(const std::string &dbPath) {
  LibraryCells libraryCells(dbPath);

  CellDB *cellDB_ptr = &cellDB;
  *cellDB_ptr = libraryCells.initializeLiberty();
}

void tech_optimize(NetID net, uint approachSelector/*, Constraints &constraints*/) {

  switch(approachSelector) {
  case 0: // cut-based matching
    //CutBasedTechMapper *mapper = new CutBasedTechMapper(cellDB);

    //MinDelay *minDelay = new MinDelay();
    //mapper->set(cellDB, strategy);

    if (model::Net::get(net).getFlipNum() != 0) {
      //NetID mappedNet = mapSequenceNet(net, mapper);
    } else {
      //NetID mappedNet = mapCombNet(net, mapper);
    }
    //mapper.techMap(net, minDelay, false);
    break;
  //case 1: // DAGON matching
  }
}

NetID mapSequenceNet(NetID netID, CutBasedTechMapper *mapper) {

  Net net = model::Net::get(netID);

  //Map связывающая cell старого net с новым mapped net
  std::map<CellID, CellID> cellMap;

  model::NetBuilder netBuilder;

  addInputToNetBuilder(netBuilder, net.getInputs(), cellMap);

  model::List<CellID> FFIDs = net.getFlipFlops();
  for (CellID FFID : FFIDs) {
    std::list<CellID> FFinputs;

    for (eda::gate::model::LinkEnd inputCell : model::Cell::get(FFID).getLinks()) {
      CellID subnetOutput = inputCell.getCellID();
      std::list<CellID> subnetInputs = getSequenceInputs(netID, subnetOutput);

      SubnetID partOfNet;// = net.getSubnet(subnetInputs, subnetOutput);

      //SubnetID mappedSubnet = mapper->techMap(partOfNet);

      // Список гейтов куда подключать отмапленую схему в netBuilder
      std::list<CellID> inputs = getNetCellID(subnetInputs, cellMap);
      CellID output; //= netBuilder.addSubnet(mappedSubnet, inputs);
      cellMap.insert(std::pair<CellID, CellID>(subnetOutput, output));

      FFinputs.push_back(subnetOutput);
    }

    std::list<CellID> inputs = getNetCellID(FFinputs, cellMap);
    CellID mappedFFID; //= addFFToNet(netBuilder, FFID, inputs);
    cellMap.insert(std::pair<CellID, CellID>(FFID, mappedFFID));
  }

  return netBuilder.make();
}

NetID mapCombNet(NetID netID, CutBasedTechMapper *mapper) {
  Net net = model::Net::get(netID);

  model::NetBuilder netBuilder;
  std::map<CellID, CellID> cellMap;

  addInputToNetBuilder(netBuilder, net.getInputs(), cellMap);

  //SubnetID mappedSubnet = mapper->techMap(net.convertNetToSubnet(net));
  //std::list<CellID> inputs = getNetCellID(net.getInputs(), cellMap);
  //netBuilder.addSubnet(mappedSubnet, inputs);

  return netBuilder.make();
}

void addInputToNetBuilder(model::NetBuilder &netBuilder,
    model::List<CellID> &inputs, std::map<CellID, CellID> &cellMap) {

  for (auto it = inputs.begin(); it != inputs.end(); ++it) {
    auto cellID = makeCell(model::IN);
    cellMap.insert(std::pair<CellID, CellID>(*it, cellID));
    netBuilder.addCell(cellID);
  }
}

std::list<CellID> getNetCellID(std::list<CellID> &subnetInputs,
std::map<CellID, CellID>  &cellMap) {
      std::list<CellID> inputs;
      for (const auto &input : subnetInputs) {
        inputs.push_back(cellMap.at(input));
      }
      return inputs;
    }

struct SubnetCellinput {
  CellID netCellID;
  size_t subnetEntryIDX;
  };
/*
SubnetID getSubnet(NetID netID, std::list<CellID> subnetInputs, CellID subnetOutput){
  using Subnet = eda::gate::model::Subnet;

  Net net = model::Net::get(netID);
  eda::gate::model::SubnetBuilder subnetBuilder;

  std::stack<CellID> stack;
  std::unordered_set<CellID> visited;

  std::vector<SubnetCellinput> subnetCellinputs;

  std::vector<CellID> netCellID;
  std::vector<size_t> subnetEntryIDX;

  stack.push(subnetOutput);
  visited.insert(subnetOutput);

    while (!stack.empty()) {
      CellID currentCellID = stack.top();
      auto currentCell = eda::gate::model::Cell::get(currentCellID);

      if (std::find(subnetInputs.begin(), subnetInputs.end(), currentCellID)) {
        auto cellID = subnetBuilder.addCell(eda::gate::model::CellSymbol::IN);
        netCellID.push_back(currentCellID);
        subnetEntryIDX.push_back(cellID);
        stack.pop();

      } else {
        bool readyForCreate = true;
        for (const auto &inputLink : currentCell.getLinks()) {
          if (netCellID.find(netCellID.begin, netCellID.end(),
              inputLink.getCellID()) == netCellID.end()) {
            readyForCreate = false;
          }
        }

        if (readyForCreate) {
          Subnet::LinkList linkList;

          for (const auto &inputLink : currentCell.getLinks()) {
            auto it = netCellID.find(netCellID.begin, netCellID.end(),
                inputLink.getCellID());
            int distance = std::distance(netCellID.begin(), it)
            Subnet::Link link(subnetEntryIDX[distance]);
            linkList.push_back(link);
          }

          auto cellID = subnetBuilder.addCell(currentCell.getTypeID(), linkList);

          netCellID.push_back(currentCellID);
          subnetEntryIDX.push_back(cellID);
          stack.pop();
        }
      }

      for (const auto &inputLink : currentCell.getLinks()) {
        auto inputCellID = inputLink.getCellID();
        if (visited.find(inputCellID) == visited.end()) {
          stack.push(inputCellID);
          visited.insert(inputCellID);
        }
      }
    }
    return subnetBuilder.make();
}
*/
} // namespace eda::gate::tech_optimizer


