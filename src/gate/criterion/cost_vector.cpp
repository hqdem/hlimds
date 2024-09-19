//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/criterion/cost_vector.h"

#include <cassert>
#include <cmath>

namespace eda::gate::criterion {

const CostVector CostVector::Zero{0., 0., 0.};
const CostVector CostVector::Unit{1., 1., 1.};

CostVector CostVector::normalize(
    const CostVector &min, const CostVector &max) const {
  /// Min/max vectors have the same size as the cost vector.
  assert(min.size() == max.size() && min.size() == size());
  return (*this - min) / (max - min);
}

CostVector CostVector::truncate(const Cost min, const Cost max) const {
  assert(min <= max);

  CostVector result;
  for (size_t i = 0; i < vector.size(); ++i) {
    result[i] = std::min(std::max(min, vector[i]), max);
  }

  return result;
}

} // namespace eda::gate::criterion
