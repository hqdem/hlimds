//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/verifier.h"

namespace eda::gate::debugger {

Verifier::Verifier(const Subnet &subnet, Solver &solver):
    encoder(SubnetEncoder::get()),
    solver(solver),
    context(subnet, solver) {
  encoder.encode(subnet, context, solver);
};

Verifier::Variable Verifier::makeEqualty(Subnet::Link lhs, bool rhs) {
  return encoder.encodeEqual(context, lhs, rhs);
}

Verifier::Variable Verifier::makeEqualty(Subnet::Link lhs, Subnet::Link rhs) {
  return encoder.encodeEqual(context, lhs, rhs);
}

bool Verifier::checkAlways(const Variable &prop, const bool invProp) {
  context.createBackup();
  addProperty(prop, !invProp);
  const bool result = !solver.solve();
  context.backup();
  return result;
}

bool Verifier::checkEventually(const Variable &prop, const bool invProp) {
  context.createBackup();
  addProperty(prop, invProp);
  const bool result = solver.solve();
  context.backup();
  return result;
}

void Verifier::addProperty(const Variable &prop, bool inv) {
  const auto &propFormula = context.getProp(prop).formula;
  for (size_t i = 0; i < propFormula.size(); ++i) {
    const auto &curClause = propFormula[i];
    Minisat::vec<Literal> clauseToAdd;
    for (const auto &lit : curClause) {
      clauseToAdd.push(lit);
    }
    solver.addClause(clauseToAdd);
  }

  solver.addClause(solver::makeLit(prop, !inv));
}

} // namespace eda::gate::debugger
