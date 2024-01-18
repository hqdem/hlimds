//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "minisat/core/Solver.h"

namespace eda::gate::solver {

//===----------------------------------------------------------------------===//
// Context
//===----------------------------------------------------------------------===//

// MiniSAT-related types.
using Variable = Minisat::Var;
using Literal  = Minisat::Lit;
using Clause   = Minisat::vec<Literal>;
using Formula  = Minisat::Solver;

inline Literal makeLit(Variable var, bool sign) {
  return Minisat::mkLit(var, sign);
}

class Solver final {
public:
  Variable newVar() {
    return formula.newVar();
  }

  Literal newLit(bool sign = 1) {
    return makeLit(newVar(), sign);
  }

  void addClause(const Clause &clause) {
    formula.addClause(clause);
  }

  void addClause(Literal l) {
    formula.addClause(l);
  }

  void addClause(Literal l1, Literal l2) {
    formula.addClause(l1, l2);
  }

  void addClause(Literal l1, Literal l2, Literal l3) {
    formula.addClause(l1, l2, l3);
  }

  void addClause(Literal l1, Literal l2, Literal l3, Literal l4) {
    Clause clause;
    clause.push(l1);
    clause.push(l2);
    clause.push(l3);
    clause.push(l4);
    addClause(clause);
  }

  void addClause(Literal l1, Literal l2, Literal l3, Literal l4, Literal l5) {
    Clause clause;
    clause.push(l1);
    clause.push(l2);
    clause.push(l3);
    clause.push(l4);
    clause.push(l5);
    addClause(clause);
  }

  void encodeBuf(Literal rhs, Literal lhs) {
    addClause(~rhs,  lhs);
    addClause( rhs, ~lhs);
  }

  void encodeAnd(Literal rhs, Literal lhs1, Literal lhs2) {
    addClause( rhs, ~lhs1, ~lhs2);
    addClause(~rhs,  lhs1);
    addClause(~rhs,  lhs2);
  }

  void encodeOr(Literal rhs, Literal lhs1, Literal lhs2) {
    addClause(~rhs,  lhs1,  lhs2);
    addClause( rhs, ~lhs1);
    addClause( rhs, ~lhs2);
  }

  void encodeXor(Literal rhs, Literal lhs1, Literal lhs2) {
    addClause(~rhs, ~lhs1, ~lhs2);
    addClause(~rhs,  lhs1,  lhs2);
    addClause( rhs, ~lhs1,  lhs2);
    addClause( rhs,  lhs1, ~lhs2);
  }

  void encodeMaj(Literal rhs, Literal lhs1, Literal lhs2, Literal lhs3) {
    auto tmp1 = newLit();
    auto tmp2 = newLit();
    auto tmp3 = newLit();

    // t1 = (x1 & x2), t2 = (x1 & x3), t3 = (x2 & x3).
    addClause(tmp1, lhs1, lhs2);
    addClause(tmp2, lhs1, lhs3);
    addClause(tmp3, lhs2, lhs3);

    // y = maj(x1, x2, x3) = (t1 | t2 | t3).
    addClause(~rhs, tmp1, tmp2, tmp3);

    addClause(rhs, ~tmp1);
    addClause(rhs, ~tmp2);
    addClause(rhs, ~tmp3);
  }

  bool solve() {
    return formula.solve();
  }

  bool solveLimited(uint64_t confBudget, uint64_t propBudget) {
    formula.setConfBudget(confBudget);
    formula.setPropBudget(propBudget);
    return formula.solve();
  }

  /// Returns the variable value (if the formula is SAT).
  bool value(Variable var) {
    return formula.modelValue(var) == Minisat::l_True;
  }

  /// Dumps the formula to the DIMACS file.
  void dump(const std::string &file) {
    formula.toDimacs(file.c_str());
  }

private:
  Formula formula;
};

} // namespace eda::gate::solver
