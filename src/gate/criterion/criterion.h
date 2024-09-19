//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/criterion/constraints.h"
#include "gate/criterion/cost_function.h"
#include "gate/criterion/penalty_function.h"

namespace eda::gate::criterion {

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
      Criterion(objective, constraints, cubicPenalty) {}

  CostVector normalize(const CostVector &vector) const {
    return vector.normalize(getMinVector(constraints),
                            getMaxVector(constraints));
  }

  Cost getCost(const CostVector &vector) const {
    return objective.function(vector);
  }

  Cost getPenalty(const CostVector &vector,
                  const CostVector &tension) const {
    return penalty(vector, tension);
  }

  Cost getPenalizedCost(const CostVector &vector,
                        const CostVector &tension) const {
    return getCost(vector) + getPenalty(vector, tension);
  }

  CostVector getTension(const CostVector &vector) const {
    const auto min = getMinVector(constraints);
    const auto max = getMaxVector(constraints);

    return vector.normalize(min, max).truncate(0.001, 1000.0);
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

} // namespace eda::gate::criterion
