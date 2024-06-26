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
	const auto prop = encoder.encodeEqual(context, solver, lhs, rhs);
	return prop;
}

Verifier::Variable Verifier::makeEqualty(Subnet::Link lhs, Subnet::Link rhs) {
	const auto prop = encoder.encodeEqual(context, solver, lhs, rhs);
	return prop;
}

bool Verifier::checkAlways(const Variable &prop, const bool invProp) {
	addProperty(prop, !invProp);
	const bool result = !solver.solve();
	solver = *saveSolver;
	delete saveSolver;
	return result;
}

bool Verifier::checkEventually(const Variable &prop, const bool invProp) {
	addProperty(prop, invProp);
	const bool result = solver.solve();
	solver = *saveSolver;
	delete saveSolver;
	return result;
}

void Verifier::addProperty(const Variable &prop, bool inv) {
	saveSolver = new Solver(solver);
	solver.addClause(solver::makeLit(prop, !inv));
}

} // namespace eda::gate::debugger
