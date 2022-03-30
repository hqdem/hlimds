//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/checker/checker.h"

namespace eda::gate::checker {

bool Checker::check(const Netlist &lhs, const Netlist &rhs) const {
  // TODO:
  return false;
}

void Checker::encode(const Netlist &net, Solver &solver) {
  for (const auto *gate: net.gates()) {
    encode(*gate, solver);
  }
}

void Checker::encode(const Gate &gate, Solver &solver) {
  switch (gate.kind()) {
  case GateSymbol::ZERO:
    encodeZero(gate, solver);
    break;
  case GateSymbol::ONE:
    encodeOne(gate, solver);
    break;
  case GateSymbol::NOP:
    encodeNop(gate, solver);
    break;
  case GateSymbol::NOT:
    encodeNot(gate, solver);
    break;
  case GateSymbol::AND:
    encodeAnd(gate, solver);
    break;
  case GateSymbol::OR:
    encodeOr(gate, solver);
    break;
  case GateSymbol::XOR:
    encodeXor(gate, solver);
    break;
  case GateSymbol::NAND:
    encodeNand(gate, solver);
    break;
  case GateSymbol::NOR:
    encodeNor(gate, solver);
    break;
  case GateSymbol::XNOR:
    encodeXnor(gate, solver);
    break;
  default:
    assert(false && "Unsupported operation");
  }
}

void Checker::encodeZero(const Gate &gate, Solver &solver) {}
void Checker::encodeOne(const Gate &gate, Solver &solver)  {}
void Checker::encodeNop(const Gate &gate, Solver &solver)  {}
void Checker::encodeNot(const Gate &gate, Solver &solver)  {}
void Checker::encodeAnd(const Gate &gate, Solver &solver)  {}
void Checker::encodeOr(const Gate &gate, Solver &solver)   {}
void Checker::encodeXor(const Gate &gate, Solver &solver)  {}
void Checker::encodeNand(const Gate &gate, Solver &solver) {}
void Checker::encodeNor(const Gate &gate, Solver &solver)  {}
void Checker::encodeXnor(const Gate &gate, Solver &solver) {}

} // namespace eda::gate::checker
