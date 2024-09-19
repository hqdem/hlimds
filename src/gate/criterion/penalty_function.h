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

/// Returns the penalty to be added to the cost.
using PenaltyFunction = std::function<Cost(const CostVector &vector,
                                           const CostVector &tension)>;

/// Always returns 0 => penalized cost = cost.
inline Cost zeroPenalty(const CostVector &vector,
                        const CostVector &tension) {
  return 0.;
}

/// Power penalty: lambda * sum{(t[i]*v[i])^p}.
inline Cost powerPenalty(const CostVector &vector,
                         const CostVector &tension,
                         const Cost power) {
  static const CostVector lambda{0.2};
  return (lambda * tension.pow(power) * vector).sum();
}

/// Linear penalty: power penalty w/ p = 1.
inline Cost linearPenalty(const CostVector &vector,
                          const CostVector &tension) {
  return powerPenalty(vector, tension, 1.);
}

/// Quadratic penalty: power penalty w/ p = 2.
inline Cost quadraticPenalty(const CostVector &vector,
                             const CostVector &tension) {
  return powerPenalty(vector, tension, 2.);
}

} // namespace eda::gate::criterion
