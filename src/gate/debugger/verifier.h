//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/utils/subnet_cnf_encoder.h"

#include <vector>

namespace eda::gate::debugger {


/**
 * @brief Allows to check different properties of provided subnet.
 */
class Verifier final {
  using Subnet = model::Subnet;
  using SubnetEncoder = model::SubnetEncoder;
  using SubnetEncoderContext = model::SubnetEncoderContext;
  using Solver = gate::solver::Solver;
  using Literal = Minisat::Lit;
  using LitVec = std::vector<Literal>;
  using Variable = Minisat::Var;

public:
  /**
   * @brief Constructs a verifier.
   *
   * @param subnet Subnet to check properties on.
   * @param solver SAT-solver.
   */
  Verifier(const Subnet &subnet, Solver &solver);

  /// Encodes and returns lhs link and rhs equivalence property.
  Variable makeEqualty(Subnet::Link lhs, bool rhs);

  /// Encodes and returns lhs and rhs links equivalence property.
  Variable makeEqualty(Subnet::Link lhs, Subnet::Link rhs);

  /**
   * @brief Checks if the provided property is always true (or false).
   *
   * @param prop Property to check.
   * @param invProp Invertes the property if set.
   */
  bool checkAlways(const Variable &prop, const bool invProp = false);

  /**
   * @brief Checks if the provided property is eventually true (or false).
   *
   * @param prop Property to check.
   * @param invProp Invertes the property if set.
   */
  bool checkEventually(const Variable &prop, const bool invProp = false);

private:
  /// Adds the provided property into the solver.
  void addProperty(const Variable &prop, bool inv);

private:
  const SubnetEncoder &encoder;
  Solver &solver;
  SubnetEncoderContext context;
};

} // namespace eda::gate::debugger
