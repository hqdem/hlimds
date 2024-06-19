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

#include <cmath>
#include <functional>

namespace eda::gate::optimizer {

/// Returns the coefficient to be multiplied to the cost.
using PenaltyFunction = std::function<Cost(const CostVector &vector,
                                           const Constraints &constraints,
                                           const CostVector &tension)>;

inline Cost zeroPenalty(const CostVector &vector,
                        const Constraints &constraints,
                        const CostVector &tension) {
  return 1.0;
}

inline Cost quadraticPenalty(const CostVector &vector,
                             const Constraints &constraints,
                             const CostVector &tension) {
  constexpr Cost alarm{0.9};
  constexpr Cost power{2.0};

  const auto min = getMinVector(constraints);
  const auto max = (getMaxVector(constraints) / tension) * alarm;

  const auto normalized = vector.normalize(min, max).truncate(0.0, 100.0);

  return 1.0 + std::pow(normalized.vector, power).sum();
}

} // namespace eda::gate::optimizer
