//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/criterion/cost_vector.h"

#include <cassert>
#include <cmath>
#include <functional>

namespace eda::gate::criterion {

/// @brief Cost function (objective).
using CostFunction = std::function<Cost(const CostVector &)>;

/// @brief Indicator identifier (index in a cost vector).
enum Indicator {
  AREA  =  0,
  DELAY =  1,
  POWER =  2,
  MIXED = -1
};

static_assert(Indicator::AREA  == 0);
static_assert(Indicator::DELAY == 1);
static_assert(Indicator::POWER == 2);

/// Aggregates two cost vector and assigns the result to the first one.
inline void aggregateCost(CostVector &result, const CostVector &vector) {
  assert(vector.size() >= CostVector::DefaultSize);

  auto &area  = result[Indicator::AREA];
  auto &delay = result[Indicator::DELAY];
  auto &power = result[Indicator::POWER];

  area  += vector[Indicator::AREA];
  delay = std::max(delay, vector[Indicator::DELAY]);
  power += vector[Indicator::POWER];
}

inline CostFunction getCostFunction(const Indicator indicator) {
  return [indicator](const CostVector &vector) -> Cost {
    return vector[indicator];
  };
}

/// @brief Objective function.
struct Objective final {
  explicit Objective(const Indicator indicator):
      indicator(indicator), function(getCostFunction(indicator)) {}

  explicit Objective(const CostFunction function):
      indicator(Indicator::MIXED), function(function) {}

  const Indicator indicator;
  const CostFunction function;
};

} // namespace eda::gate::criterion
