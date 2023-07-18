//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/bdd_checker.h"

namespace eda::gate::debugger {

CheckerResult bddChecker(GNet &net1, GNet &net2, Hints &hints) {
  if (!(net1.isComb() && net2.isComb())) {
    return CheckerResult::ERROR;
  }

  GNet *miterNet = miter(net1, net2, hints);
  SignalList inputs;
  GateId outputId = 0;

  Cudd manager(0, 0);
  for (Gate *gate : miterNet->gates()) {
    if (gate->func() == GateSymbol::IN) {
      inputs.push_back(Signal::always(gate->id()));
    }
    if (gate->func() == GateSymbol::OUT) {
      outputId = gate->id();
    }
  }
  BddList bddVarList = {};
  for (unsigned i = 0; i < inputs.size(); i++) {
    bddVarList.push_back(manager.bddVar());
  }
  
  GateBDDMap varMap;
  for (unsigned i = 0; i < bddVarList.size(); i++) {
    varMap[inputs[i].node()] = bddVarList[i];
  }

  BDD netBDD = GNetBDDConverter::convert(*miterNet, outputId, varMap, manager);
  if (netBDD == manager.bddZero()) {
    return CheckerResult::EQUAL;
  }
  return CheckerResult::NOTEQUAL;
} 

CheckerResult BddChecker::equivalent(GNet &lhs,
                                     GNet &rhs,
                                     Checker::GateIdMap &gmap) {
  Checker::Hints hints = makeHints(lhs, rhs, gmap);
  return bddChecker(lhs, rhs, hints);
}

} // namespace eda::gate::debugger
