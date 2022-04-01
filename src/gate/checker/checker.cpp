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

//===----------------------------------------------------------------------===//
// MiniSat-Related Functions
//===----------------------------------------------------------------------===//

/// Returns a new variable.
unsigned new_var() {
  static unsigned id = 1024*1024;
  return id++;
}

/// Creates a literal.
inline Minisat::Lit lit(unsigned id, bool sign) {
  return Minisat::mkLit(static_cast<Minisat::Var>(id), sign);
}

/// Creates a variable.
inline unsigned var(unsigned offset, const Gate &gate) {
  return offset + gate.id();
}

/// Creates a variable.
inline unsigned var(unsigned offset, const Signal &signal) {
  return offset + signal.gate()->id();
}

/// Encodes the equality y^s == x.
inline void encode_buf(unsigned y, unsigned x,
                      bool sign, Minisat::Solver &solver) {
  solver.addClause(lit(x, true), lit(y, !sign));
  solver.addClause(lit(x, false), lit(y, sign));
}

/// Encodes the equality y^s == x1 ^ x2.
inline void encode_xor(unsigned y, unsigned x1, unsigned x2,
                      bool sign, Minisat::Solver &solver) {
  solver.addClause(lit(y, sign), lit(x1, true), lit(x2, true));
  solver.addClause(lit(y, sign), lit(x1, false), lit(x2, false));
  solver.addClause(lit(y, !sign), lit(x1, true), lit(x2, false));
  solver.addClause(lit(y, !sign), lit(x1, false), lit(x2, true));
}

//===----------------------------------------------------------------------===//
// Equivalence Checker
//===----------------------------------------------------------------------===//

bool Checker::equiv(const Netlist &lhs,
                    const Netlist &rhs,
                    const std::vector<std::pair<Gate*,Gate*>> &imap,
                    const std::vector<std::pair<Gate*,Gate*>> &omap) const {
  Minisat::Solver solver;

  const unsigned lhsOffset = 0;
  const unsigned rhsOffset = lhs.size();

  // Encode the netlists.
  encode(lhsOffset, lhs, solver);
  encode(rhsOffset, rhs, solver);

  // Equate the inputs.
  for (const auto &[lhsGate, rhsGate] : imap) {
    const unsigned x = var(lhsOffset, *lhsGate);
    const unsigned y = var(rhsOffset, *rhsGate);

    encode_buf(y, x, true, solver);
  }

  // Compare the outputs.
  Minisat::vec<Minisat::Lit> existsDiff(omap.size());
  for (const auto &[lhsGate, rhsGate] : omap) {
    const unsigned y  = new_var();
    const unsigned x1 = var(lhsOffset, *lhsGate);
    const unsigned x2 = var(rhsOffset, *rhsGate);

    encode_xor(y, x1, x2, true, solver);
    existsDiff.push(lit(y, true));
  }

  // (lOut[1] != rOut[1]) || ... || (lOut[m] != rOut[m]).
  solver.addClause(existsDiff);

  return !solver.solve();
}

//===----------------------------------------------------------------------===//
// Tseitin Encoding
//===----------------------------------------------------------------------===//

void Checker::encode(unsigned offset, const Netlist &net, Minisat::Solver &solver) const {
  for (const auto *gate: net.gates())
    encode(offset, *gate, solver);
}

void Checker::encode(unsigned offset, const Gate &gate, Minisat::Solver &solver) const {
  if (gate.is_source())
    return;

  switch (gate.kind()) {
  case GateSymbol::ONE:
    encodeFix(offset, gate, true, solver);
    break;
  case GateSymbol::ZERO:
    encodeFix(offset, gate, false, solver);
    break;
  case GateSymbol::NOP:
    encodeBuf(offset, gate, true, solver);
    break;
  case GateSymbol::NOT:
    encodeBuf(offset, gate, false, solver);
    break;
  case GateSymbol::AND:
    encodeAnd(offset, gate, true, solver);
    break;
   case GateSymbol::NAND:
    encodeAnd(offset, gate, false, solver);
    break;
  case GateSymbol::OR:
    encodeOr(offset, gate, true, solver);
    break;
  case GateSymbol::NOR:
    encodeOr(offset, gate, false, solver);
    break;
  case GateSymbol::XOR:
    encodeXor(offset, gate, true, solver);
    break;
  case GateSymbol::XNOR:
    encodeXor(offset, gate, false, solver);
    break;
  default:
    assert(false && "Unsupported operation");
  }
}

void Checker::encodeFix(unsigned offset, const Gate &gate, bool sign, Minisat::Solver &solver) const {
  solver.addClause(lit(var(offset, gate), sign));
}

void Checker::encodeBuf(unsigned offset, const Gate &gate, bool sign, Minisat::Solver &solver) const {
  const unsigned x = var(offset, gate.input(0));
  const unsigned y = var(offset, gate);

  encode_buf(y, x, sign, solver);
}

void Checker::encodeAnd(unsigned offset, const Gate &gate, bool sign, Minisat::Solver &solver) const {
  const unsigned y = var(offset, gate);
  Minisat::vec<Minisat::Lit> clause(gate.arity() + 1);
  
  clause.push(lit(y, sign));
  for (const auto &input : gate.inputs()) {
    const unsigned x = var(offset, input);

    clause.push(lit(x, false));
    solver.addClause(lit(y, !sign), lit(x, true));
  }

  solver.addClause(clause);
}

void Checker::encodeOr(unsigned offset, const Gate &gate, bool sign, Minisat::Solver &solver) const {
  const unsigned y = var(offset, gate);
  Minisat::vec<Minisat::Lit> clause(gate.arity() + 1);
  
  clause.push(lit(y, !sign));
  for (const auto &input : gate.inputs()) {
    const unsigned x = var(offset, input);

    clause.push(lit(x, true));
    solver.addClause(lit(y, sign), lit(x, false));
  }

  solver.addClause(clause);
}

void Checker::encodeXor(unsigned offset, const Gate &gate, bool sign, Minisat::Solver &solver) const {
  if (gate.arity() == 1)
    return encodeBuf(offset, gate, sign, solver);

  unsigned y = var(offset, gate);
  for (unsigned i = 0; i < gate.arity() - 1; i++) {
    const unsigned x1 = var(offset, gate.input(i));
    const unsigned x2 = (i == gate.arity() - 2)
      ? var(offset, gate.input(i + 1))
      : new_var();

    encode_xor(y, x1, x2, sign, solver);
    y = x2;
  }
}

} // namespace eda::gate::checker
