//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/bdd_checker.h"

namespace eda::gate::debugger {

CheckerResult BddChecker::equivalent(const GNet &lhs,
                                     const GNet &rhs,
                                     const SatChecker::GateIdMap &gmap) const {
  if (!(lhs.isComb() && rhs.isComb())) {
    return CheckerResult::ERROR;
  }

  GNet *miterNet = miter(lhs, rhs, gmap);
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

} // namespace eda::gate::debugger
