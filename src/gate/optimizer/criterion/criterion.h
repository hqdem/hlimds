//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/criterion/constraints.h"
#include "gate/optimizer/criterion/cost_function.h"
#include "gate/optimizer/criterion/penalty_function.h"

namespace eda::gate::optimizer {

/// @brief Optimization criterion w/ constraints.
struct Criterion final {
  Criterion(const Objective &objective,
            const Constraints &constraints,
            const PenaltyFunction penalty):
      objective(objective),
      constraints(constraints),
      penalty(penalty) {}

  Criterion(const Objective &objective,
            const Constraints &constraints):
      Criterion(objective, constraints, quadraticPenalty) {}

  CostVector normalize(const CostVector &vector) const {
    return vector.normalize(getMinVector(constraints),
                            getMaxVector(constraints));
  }

  Cost getCost(const CostVector &vector) const {
    return objective.function(vector);
  }

  Cost getPenalty(const CostVector &vector) const {
    return penalty(vector, constraints);
  }

  Cost getPenalizedCost(const CostVector &vector) const {
    return getCost(vector) * getPenalty(vector);
  }

  bool check(const CostVector &vector) const {
    for (const auto &constraint : constraints) {
      if (!constraint.check(vector)) {
        return false;
      }
    }
    return true;
  }

  /// Objective function.
  const Objective objective;
  /// Design constraints.
  const Constraints constraints;
  /// Penalty function.
  const PenaltyFunction penalty;
};

} // namespace eda::gate::optimizer
