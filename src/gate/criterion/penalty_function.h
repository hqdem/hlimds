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

#include <cmath>
#include <functional>

namespace eda::gate::criterion {

/// Returns the coefficient to be multiplied to the cost.
using PenaltyFunction = std::function<Cost(const CostVector &vector,
                                           const Constraints &constraints,
                                           const CostVector &tension)>;

/// Always returns 1 => penaltized cost = cost.
inline Cost zeroPenalty(const CostVector &vector,
                        const Constraints &constraints,
                        const CostVector &tension) {
  return 1.;
}

/// Quadratic penalty.
inline Cost quadraticPenalty(const CostVector &vector,
                             const Constraints &constraints,
                             const CostVector &tension) {
  constexpr Cost alarm{0.9};

  const auto min = getMinVector(constraints);
  const auto max = getMaxVector(constraints) / (tension + 1.f / alarm);

  const auto normalized = vector.normalize(min, max).truncate(0., 100.);

  return 1. + std::pow(normalized.vector, 2.).sum();
}

} // namespace eda::gate::criterion
