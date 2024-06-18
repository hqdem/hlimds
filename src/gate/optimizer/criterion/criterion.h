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
#include <valarray>
#include <vector>

namespace eda::gate::optimizer {

//===----------------------------------------------------------------------===//
// Cost Function
//===----------------------------------------------------------------------===//

/// @brief Cost datatype (must meet the NumericType requirements).
using Cost = float;

/// @brief Stores the estimated (predicted) design characteristics.
struct CostVector final {
  /// Area, delay, and power.
  static constexpr size_t DefaultSize = 3;

  /// Zero cost vector.
  static CostVector Zero;

  explicit CostVector(const size_t size):
      vector(size) {}
  explicit CostVector(const std::valarray<Cost> &vector):
      vector(vector) {}
  CostVector():
      CostVector(DefaultSize) {}

  size_t size() const {
    return vector.size();
  }

  const Cost &operator[](const size_t i) const {
    return vector[i];
  }

  Cost &operator[](const size_t i) {
    return vector[i];
  }

  CostVector operator+(const CostVector &other) const {
    return CostVector{vector + other.vector};
  }

  CostVector operator-(const CostVector &other) const {
    return CostVector{vector - other.vector};
  }

  CostVector operator*(const CostVector &other) const {
    return CostVector{vector * other.vector};
  }

  CostVector operator/(const CostVector &other) const {
    return CostVector{vector / other.vector};
  }

  CostVector operator*(const Cost coefficient) const {
    return CostVector{vector * coefficient};
  }

  CostVector operator/(const Cost coefficient) const {
    return CostVector{vector / coefficient};
  }

  CostVector normalize(const CostVector &min, const CostVector &max) const;

  CostVector truncate(const Cost min, const Cost max) const;

  std::valarray<Cost> vector;
};

/// @brief Cost function (objective).
using CostFunction = std::function<Cost(const CostVector &)>;

/// @brief Indicator identifier (index in a cost vector).
enum Indicator {
  AREA  = 0,
  DELAY = 1,
  POWER = 2,
  MIXED = -1
};

inline CostFunction getCostFunction(const Indicator indicator) {
  return [indicator](const CostVector &vector) {
    return vector[indicator];
  };
}

//===----------------------------------------------------------------------===//
// Constraints
//===----------------------------------------------------------------------===//

/// @brief Min-max constraint for an indicator.
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

  bool check(const Cost cost) const {
    return min <= cost && cost <= max;
  }

  bool check(const CostVector &vector) const {
    return check(function(vector));
  }

  const CostFunction function;

  const Cost min;
  const Cost max;
};

using Constraints = std::vector<Constraint>;

CostVector getMinVector(const Constraints &constraints);
CostVector getMaxVector(const Constraints &constraints);

/// @brief Objective function.
struct Objective final {
  explicit Objective(const Indicator indicator):
    indicator(indicator), function(getCostFunction(indicator)) {}

  explicit Objective(const CostFunction function):
    indicator(MIXED), function(function) {}

  const Indicator indicator;
  const CostFunction function;
};

//===----------------------------------------------------------------------===//
// Penalty Function
//===----------------------------------------------------------------------===//

/// Returns the coefficient to be multiplied w/ the cost.
using PenaltyFunction =
    std::function<Cost(const CostVector &, const Constraints &)>;

inline Cost zeroPenalty(const CostVector &, const Constraints &) {
  return 1.0;
}

Cost quadraticPenalty(const CostVector &vector, const Constraints &constraints);

//===----------------------------------------------------------------------===//
// Criterion
//===----------------------------------------------------------------------===//

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
