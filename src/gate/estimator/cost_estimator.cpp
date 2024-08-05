//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/estimator/cost_estimator.h"

#include <cassert>
#include <cmath>

namespace eda::gate::estimator {

static inline void aggregateCost(criterion::CostVector &result,
                                 const criterion::CostVector &vector) {
  assert(vector.size() >= criterion::CostVector::DefaultSize);

  auto &area  = result[criterion::AREA];
  auto &delay = result[criterion::DELAY];
  auto &power = result[criterion::POWER];

  area  += vector[criterion::AREA];
  delay = std::max(delay, vector[criterion::DELAY]);
  power += vector[criterion::POWER];
}

criterion::CostVector CostAggregator::getCost(
    const model::DesignBuilder &design) const {
  auto designBuilder = const_cast<model::DesignBuilder&>(design);
  auto designCost = criterion::CostVector::Zero;

  for (size_t i = 0; i < design.getSubnetNum(); ++i) {
    const auto subnetBuilder = designBuilder.getSubnetBuilder(i);
    const auto subnetCost = subnetEstimator.getCost(*subnetBuilder);

    aggregateCost(designCost, subnetCost);
  }

  return designCost;
}

} // namespace eda::gate::estimator
