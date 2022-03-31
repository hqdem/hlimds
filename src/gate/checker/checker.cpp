//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/checker/checker.h"

#include "minisat/core/Solver.h"

namespace eda::gate::checker {

bool Checker::check(const Netlist &lhs, const Netlist &rhs) const {
  // TODO:
  return false;
}

void Checker::encode(const Netlist &net, Minisat::Solver &solver) {
  for (const auto *gate: net.gates()) {
    encode(*gate, solver);
  }
}

void Checker::encode(const Gate &gate, Minisat::Solver &solver) {
  switch (gate.kind()) {
  case GateSymbol::ONE:
    encodeFix(gate, true, solver);
    break;
  case GateSymbol::ZERO:
    encodeFix(gate, false, solver);
    break;
  case GateSymbol::NOP:
    encodeBuf(gate, true, solver);
    break;
  case GateSymbol::NOT:
    encodeBuf(gate, false, solver);
    break;
  case GateSymbol::AND:
    encodeAnd(gate, true, solver);
    break;
   case GateSymbol::NAND:
    encodeAnd(gate, false, solver);
    break;
  case GateSymbol::OR:
    encodeOr(gate, true, solver);
    break;
  case GateSymbol::NOR:
    encodeOr(gate, false, solver);
    break;
  case GateSymbol::XOR:
    encodeXor(gate, true, solver);
    break;
  case GateSymbol::XNOR:
    encodeXor(gate, false, solver);
    break;
  default:
    assert(false && "Unsupported operation");
  }
}

inline Minisat::Lit lit(unsigned id, bool sign) {
  return Minisat::mkLit(static_cast<Minisat::Var>(id), sign);
}

void Checker::encodeFix(const Gate &gate, bool sign, Minisat::Solver &solver) {
  solver.addClause(lit(gate.id(), sign));
}

void Checker::encodeBuf(const Gate &gate, bool sign, Minisat::Solver &solver) {
  const unsigned x = gate.input(0).gate()->id();
  const unsigned y = gate.id();

  solver.addClause(lit(x, true), lit(y, !sign));
  solver.addClause(lit(x, false), lit(y, sign));
}

void Checker::encodeAnd(const Gate &gate, bool sign, Minisat::Solver &solver) {
  const unsigned y = gate.id();
  Minisat::vec<Minisat::Lit> clause(gate.arity() + 1);
  
  clause.push(lit(y, sign));
  for (const auto &input : gate.inputs()) {
    const unsigned x = input.gate()->id();

    clause.push(lit(x, false));
    solver.addClause(lit(y, !sign), lit(x, true));
  }

  solver.addClause(clause);
}

void Checker::encodeOr(const Gate &gate, bool sign, Minisat::Solver &solver) {
  const unsigned y = gate.id();
  Minisat::vec<Minisat::Lit> clause(gate.arity() + 1);
  
  clause.push(lit(y, !sign));
  for (const auto &input : gate.inputs()) {
    const unsigned x = input.gate()->id();

    clause.push(lit(x, true));
    solver.addClause(lit(y, sign), lit(x, false));
  }

  solver.addClause(clause);
}

void Checker::encodeXor(const Gate &gate, bool sign, Minisat::Solver &solver) {
  if (gate.arity() == 1)
    return encodeBuf(gate, sign, solver);

  unsigned y = gate.id();
  for (unsigned i = 0; i < gate.arity() - 1; i++) {
    const unsigned x1 = gate.input(i).gate()->id();
    const unsigned x2 = (i == gate.arity() - 2)
      ? gate.input(i + 1).gate()->id()
      : getNewVar();

    solver.addClause(lit(y, sign), lit(x1, true), lit(x2, true));
    solver.addClause(lit(y, sign), lit(x1, false), lit(x2, false));
    solver.addClause(lit(y, !sign), lit(x1, true), lit(x2, false));
    solver.addClause(lit(y, !sign), lit(x1, false), lit(x2, true));

    y = x2;
  }
}

unsigned Checker::getNewVar() {
  static unsigned id = 1024*1024;
  return id++;
}

} // namespace eda::gate::checker
