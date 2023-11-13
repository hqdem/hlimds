//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/celltype.h"
#include "gate/techoptimizer/cut_based_tech_mapper/cut_based_tech_mapper.h"
#include "gate/techoptimizer/cut_based_tech_mapper/strategy/min_delay.h"
#include "gate/techoptimizer/cut_based_tech_mapper/strategy/strategy.h"
#include "gate/techoptimizer/library/cell.h"
#include "gate/techoptimizer/tech_optimizer.h"
#include "gate/techoptimizer/library/subnetattr.h"

namespace eda::gate::tech_optimizer {

std::unordered_map<std::string, eda::gate::model::CellTypeID> cellTypeMap;
eda::gate::optimizer::SQLiteRWDatabase functDB;
eda::gate::optimizer::SQLiteRWDatabase structDB;
std::unordered_map<eda::gate::model::SubnetID, Subnetattr> cellDB;

void read_db(const std::string &dbPath) {
  // Read and populate the databases. Input format: Liberty.
  // 1. Populate functDB
  // 2. Populate structDB
  // TODO: account for latches and FFs, also

  LibraryCells libraryCells(dbPath);

    functDB.linkDB("rwtest.db");
    functDB.openDB();

    libraryCells.initializeLibraryRwDatabase(&functDB, cellTypeMap);
}

void tech_optimize(GNet *net, uint approachSelector/*, Constraints &constraints*/) { // TODO: approach selector should be enum
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
} // namespace eda::gate::tech_optimizer


