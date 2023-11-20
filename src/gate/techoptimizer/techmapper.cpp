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

void tech_optimize(NetID net, uint approachSelector/*, Constraints &constraints*/) { // TODO: approach selector should be enum
  // Run the stage of technology mapping to construct
  // a cell network by means of a cell library.

  /*
  Strategy strategy; // TODO this strategy should be selected basing on approachSelector
  */

  // 1. Cut-based matching.
  // 2. Another (structural) approach: say, DAGON.
  //     a. Rule-based (GREGORY, 1986)
  //     b. Graph matching (KEUTZER, 1987)
  //     c. Direct mapping (LEGA, 1988)
  
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

  // Run the stage of technology-dependent optimizations to obtain
  // an optimized cell network.

  // Here one needs to run STA on the constructed network, estimate the
  // parameters of the network and if the constrains are violated,
  // update the network.

  // 1. Gate sizing.
  // 2. ?

  // TODO: conceptional sequence of the next steps.
    //OpenSTA sta(); // e.g., OpenSTA
    //GateSizer gateSizer();
  //
  //do {
  //  gateSizer(net);
  //} while (!sta.check(net, constraints));

  // Final checks
  // TODO: checking for functional equivalence
  //functionalEquivalence
}

void sequence_net(NetID net) {
  List<CellID> FFIDs = model::Net::get(net).getFlipFlops();

  for (CellID FFID : FFIDs) {
    for (Cell::get)
        std::cout << elem << " ";
    }
}
} // namespace eda::gate::tech_optimizer


