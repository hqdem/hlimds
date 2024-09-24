//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <cassert>
#include <cmath>
#include <cstddef>
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
  /// Unit cost vector.
  static const CostVector Unit;

  explicit CostVector():
      vector(DefaultSize) {}

  explicit CostVector(const std::valarray<Cost> &vector):
      vector(vector) {}

  explicit CostVector(const std::valarray<Cost> &&vector) noexcept:
      vector(std::move(vector)) {}

  CostVector(const Cost a, const Cost d, const Cost p):
      CostVector(std::valarray<Cost>{a, d, p}) {}

  CostVector(const Cost x):
      CostVector(x, x, x) {}

  CostVector(const CostVector &vector):
      CostVector(vector.vector) {}

  CostVector(CostVector &&vector) noexcept:
      CostVector(std::move(vector.vector)) {}

  size_t size() const {
    return vector.size();
  }

  const Cost &operator[](const size_t i) const {
    return vector[i];
  }

  Cost &operator[](const size_t i) {
    return vector[i];
  }

  Cost sum() const {
    return vector.sum();
  }

  Cost norm(const Cost p) const {
    return std::pow(abs().pow(p).sum(), 1. / p);
  }

  CostVector operator+(const CostVector &other) const {
    return CostVector(vector + other.vector);
  }

  CostVector operator-(const CostVector &other) const {
    return CostVector(vector - other.vector);
  }

  CostVector operator*(const CostVector &other) const {
    return CostVector(vector * other.vector);
  }

  CostVector operator/(const CostVector &other) const {
    return CostVector(vector / other.vector);
  }

  CostVector operator-(const Cost other) const {
    return CostVector(vector - other);
  }

  CostVector operator+(const Cost other) const {
    return CostVector(vector + other);
  }

  CostVector operator*(const Cost other) const {
    return CostVector(vector * other);
  }

  CostVector operator/(const Cost other) const {
    return CostVector(vector / other);
  }

  CostVector abs() const {
    return CostVector(std::abs(vector));
  }

  CostVector pow(const Cost power) const {
    return CostVector(std::pow(vector, power));
  }

  CostVector exp() const {
    return CostVector(std::exp(vector));
  }

  CostVector softmax(const Cost tau) const {
    const auto v = std::exp(vector / tau);
    return CostVector(v / v.sum());
  }

  CostVector smooth(const CostVector &pivot, const Cost alpha) const {
    return CostVector(vector * alpha + pivot.vector * (1. - alpha));
  }

  Cost dot(const CostVector &other) const {
    assert(vector.size() == other.vector.size());
    Cost result = .0;
    for (size_t i = 0; i < vector.size(); ++i) {
      result += vector[i] * other.vector[i];
    }
    return result;
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

  CostVector &operator+=(const Cost other) {
    vector += other;
    return *this;
  }

  CostVector &operator-=(const Cost other) {
    vector -= other;
    return *this;
  }

  CostVector &operator*=(const Cost other) {
    vector *= other;
    return *this;
  }

  CostVector &operator/=(const Cost other) {
    vector /= other;
    return *this;
  }

  CostVector normalize(const CostVector &min, const CostVector &max) const;

  CostVector truncate(const Cost min, const Cost max) const;

private:
  std::valarray<Cost> vector;
};

} // namespace eda::gate::criterion
