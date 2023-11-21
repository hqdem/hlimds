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
#include "gate/techoptimizer/techmapper.h"

namespace eda::gate::tech_optimizer {

CellDB cellDB;

void read_db(const std::string &dbPath) {
  // Read and populate the databases. Input format: Liberty.
  // 1. Populate functDB
  // 2. Populate structDB
  // TODO: account for latches and FFs, also

  LibraryCells libraryCells(dbPath);

  CellDB* cellDB_ptr = &cellDB;
  *cellDB_ptr = libraryCells.initializeLiberty();
}

void tech_optimize(NetID net, uint approachSelector/*, Constraints &constraints*/) {
  
  switch(approachSelector) {
  case 0: // cut-based matching
    CutBasedTechMapper mapper(functDB, cellTypeMap);
    MinDelay *minDelay = new MinDelay();
    mapper.techMap(net, minDelay, false); // TODO not need to return the pointer
  break;
  //case 1: // DAGON matching
    //DagonTechMapper mapper(structDB);
    //mapper.techMap(net, &strategy);
  }
}

void sequence_net(NetID netID) {
  Net net = model::Net::get(netID);
  List<CellID> FFIDs = net.getFlipFlops();

  for (CellID FFID : FFIDs) {
    for (eda::gate::model::LinkEnd inputCell : Cell::get(FFID).getLinks()) {

      CellID subnetOutput = inputCell.getCellID();
      std::list<CellID> subnetInputs = getSequenceInputs(netID, subnetOutput);
      SubnetID subnetID = net.getSubnet(subnetInputs, subnetOutput);

      

      std::cout << elem << " ";
    }
  }
}
} // namespace eda::gate::tech_optimizer


