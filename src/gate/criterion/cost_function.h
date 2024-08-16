//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <cstddef>
#include <functional>
#include <valarray>

namespace eda::gate::criterion {

/// @brief Cost datatype (must meet the NumericType requirements).
using Cost = float;

/// @brief Stores the estimated (predicted) design characteristics.
struct CostVector final {
  /// Area, delay, and power.
  static constexpr size_t DefaultSize = 3;

  /// Zero cost vector.
  static const CostVector Zero;

  explicit CostVector():
      vector(DefaultSize) {}

  explicit CostVector(const std::valarray<Cost> &vector):
      vector(vector) {}

  CostVector(const Cost a, const Cost d, const Cost p):
      CostVector(std::valarray<Cost>{a, d, p}) {}

  CostVector(const CostVector &vector):
      CostVector(vector.vector) {}

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

  CostVector &operator=(const CostVector &other) {
    vector = other.vector;
    return *this;
  }

  CostVector &operator+=(const CostVector &other) {
    vector += other.vector;
    return *this;
  }

  CostVector &operator-=(const CostVector &other) {
    vector -= other.vector;
    return *this;
  }

  CostVector &operator*=(const CostVector &other) {
    vector *= other.vector;
    return *this;
  }

  CostVector &operator/=(const CostVector &other) {
    vector /= other.vector;
    return *this;
  }

  CostVector normalize(const CostVector &min, const CostVector &max) const;

  CostVector truncate(const Cost min, const Cost max) const;

  std::valarray<Cost> vector;
};

/// @brief Cost function (objective).
using CostFunction = std::function<Cost(const CostVector &)>;

/// @brief Indicator identifier (index in a cost vector).
enum Indicator {
  AREA  =  0,
  DELAY =  1,
  POWER =  2,
  MIXED = -1
};

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
    indicator(MIXED), function(function) {}

  const Indicator indicator;
  const CostFunction function;
};

} // namespace eda::gate::criterion
