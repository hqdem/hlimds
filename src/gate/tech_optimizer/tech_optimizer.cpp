//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/tech_optimizer/tech_optimizer.h"
#include "gate/tech_mapper/strategy/strategy.h"

namespace eda::gate::tech_optimizer {

// TODO: do we indeed need it in such a format?
// what if we use a structural matching?
eda::gate::optimizer::SQLiteRWDatabase functDB;
eda::gate::optimizer::SQLiteRWDatabase structDB; // TODO: replace the format

void read_db(std::string &dbPath) {
  // Read and populate the databases. Input format: Liberty.
  // 1. Populate functDB
  // 2. Populate structDB
  // TODO: account for latches and FFs, also
}

void tech_optimize(GNet *net, uint approachSelector) { // TODO: approach selector should be enum
  // Run the stage of technology mapping to construct
  // a cell network by means of a cell library.

  Strategy strategy; // TODO this strategy should be selected basing on approachSelector

  // 1. Cut-based matching.
  // 2. Another (structural) approach: say, DAGON.
  //     a. Rule-based (GREGORY, 1986)
  //     b. Graph matching (KEUTZER, 1987)
  //     c. Direct mapping (LEGA, 1988)
  
  switch(approachSelector) {
  case 0: // cut-based matching
    CutBasedTechMapper mapper(functDB);
    mapper.techMap(net, &strategy); // TODO not need to return the pointer
  break;
  case 1: // DAGON matching
    DagonTechMapper mapper(structDB);
    mapper.techMap(net, &strategy);
  }

  // Run the stage of technology-dependent optimizations to obtain
  // an optimized cell network.

  // Here one needs to run STA on the constructed network, estimate the
  // parameters of the network and if the constrains are violated,
  // update the network.

  // 1. Gate sizing.
  // 2. ?

  // TODO: conceptional sequence of the next steps.
  // STA sta(); // e.g., OpenSTA
  // GateSizer gateSizer();
  //
  // do {
  //   gateSizer(net);
  // } while (!sta.check(net, constraints));

  // Final checks
  // TODO: checking for functional equivalence
}