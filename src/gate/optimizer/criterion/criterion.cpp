//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/criterion/criterion.h"

namespace eda::gate::optimizer {

CostVector CostVector::Zero{};

CostVector CostVector::normalize(
    const CostVector &min, const CostVector &max) const {
  /// Min/max vectors have the same size as the cost vector.
  assert(min.size() == max.size() && min.size() == size());
  return (*this - min) / (max - min);
}

CostVector CostVector::truncate(const float min, const float max) const {
  assert(min <= max);

  CostVector result(vector.size());
  for (size_t i = 0; i < vector.size(); ++i) {
    result[i] = std::min(std::max(min, vector[i]), max);
  }

  return result;
}

CostVector getMinVector(const Constraints &constraints) {
  CostVector result(constraints.size());
  for (size_t i = 0; i < constraints.size(); ++i) {
    result[i] = constraints[i].min;
  }
  return result;
}

CostVector getMaxVector(const Constraints &constraints) {
  CostVector result(constraints.size());
  for (size_t i = 0; i < constraints.size(); ++i) {
    result[i] = constraints[i].max;
  }
  return result;
}

Cost quadPenalty(const CostVector &vector, const Constraints &constraints) {
  constexpr Cost alarm{0.9};
  constexpr Cost power{2.0};

  const auto min = getMinVector(constraints);
  const auto max = getMaxVector(constraints) * alarm;

  const auto normalized = vector.normalize(min, max).truncate(0.0, 100.0);

  return 1.0 + std::pow(normalized.vector, power).sum();
}

} // namespace eda::gate::optimizer
