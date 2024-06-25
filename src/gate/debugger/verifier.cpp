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

Verifier::Property Verifier::getProperty(const Variable var) const {
	return prop.find(var)->second;
}

Verifier::Variable Verifier::makeEqualty(Subnet::Link lhs, bool rhs) {
	const Property newProp = encoder.encodeEqual(context, lhs, rhs);
	prop[newProp.p] = newProp;
	return newProp.p;
}

Verifier::Variable Verifier::makeEqualty(Subnet::Link lhs, Subnet::Link rhs) {
	const Property newProp = encoder.encodeEqual(context, lhs, rhs);
	prop[newProp.p] = newProp;
	return newProp.p;
}

bool Verifier::checkAlways(const Property &prop, const bool invProp) {
	addClauses(prop, !invProp);
	const bool result = !solver.solve();
	solver = *saveSolver;
	delete saveSolver;
	return result;
}

bool Verifier::checkEventually(const Property &prop, const bool invProp) {
	addClauses(prop, invProp);
	const bool result = solver.solve();
	solver = *saveSolver;
	delete saveSolver;
	return result;
}

void Verifier::addClauses(const Property &prop, bool inv) {
	if (!prop.addedToFormula) {
		const auto &propFormula = prop.formula;
		for (size_t i = 0; i < propFormula.size(); ++i) {
			const auto &curClause = propFormula[i];
			Minisat::vec<Literal> clauseToAdd;
			for (const auto &lit : curClause) {
				clauseToAdd.push(lit);
			}
			solver.addClause(clauseToAdd);
		}
		prop.addedToFormula = true;
	}
	saveSolver = new Solver(solver);
	solver.addClause(solver::makeLit(prop.p, !inv));
}

} // namespace eda::gate::debugger
