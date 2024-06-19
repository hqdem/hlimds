//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/criterion/cost_function.h"

#include <cassert>
#include <cstddef>
#include <vector>

namespace eda::gate::optimizer {

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

inline CostVector getMinVector(const Constraints &constraints) {
  assert(constraints.size() == CostVector::DefaultSize);

  CostVector result;
  for (size_t i = 0; i < constraints.size(); ++i) {
    result[i] = constraints[i].min;
  }
  return result;
}

inline CostVector getMaxVector(const Constraints &constraints) {
  assert(constraints.size() == CostVector::DefaultSize);

  CostVector result;
  for (size_t i = 0; i < constraints.size(); ++i) {
    result[i] = constraints[i].max;
  }
  return result;
}

} // namespace eda::gate::optimizer
