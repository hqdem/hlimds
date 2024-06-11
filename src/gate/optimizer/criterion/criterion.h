//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace eda::gate::optimizer {

using Cost = float;
using CostVector = std::vector<Cost>;
using CostFunction = std::function<float(const CostVector &)>;

/// @brief Index in cost vector.
enum Indicator {
  AREA  = 0,
  DELAY = 1,
  POWER = 2,
  MIXED = -1
};

extern CostVector ZeroVector;

inline CostFunction getCostFunction(const Indicator indicator) {
  return [indicator](const CostVector &vector) {
    return vector[indicator];
  };
}

/// @brief Min-max constraint for a characteristic.
struct Constraint final {
  Constraint(const CostFunction function, const Cost min, const Cost max):
      function(function), min(min), max(max) {
    assert(0.0 <= min && min <= max);
  } 
 
  Constraint(const CostFunction function, const Cost max):
      Constraint(function, 0.0, max) {}

  Constraint(const Indicator indicator, const Cost min, const Cost max):
      Constraint(getCostFunction(indicator), min, max) {
    assert(indicator != MIXED);
  }

  Constraint(const Indicator indicator, const Cost max):
      Constraint(indicator, 0.0, max) {}

  const CostFunction function;

  const Cost min;
  const Cost max;

  bool check(const Cost cost) const {
    return min <= cost && cost <= max;
  }

  bool check(const CostVector &vector) const {
    return check(function(vector));
  }
};

using Constraints = std::vector<Constraint>;

/// @brief Objective function.
struct Objective final {
  explicit Objective(const Indicator indicator):
    indicator(indicator), function(getCostFunction(indicator)) {}

  explicit Objective(const CostFunction function):
    indicator(MIXED), function(function) {}

  const Indicator indicator;
  const CostFunction function;
};

/// @brief Optimization criterion w/ constraints.
struct Criterion final {
  const Objective objective;
  const Constraints constraints;

  Cost cost(const CostVector &vector) const {
    return objective.function(vector);
  }

  bool check(const CostVector &vector) const {
    for (const auto &constraint : constraints) {
      if (!constraint.check(vector)) {
        return false;
      }
    }
    return true;
  }
};

} // namespace eda::gate::optimizer
