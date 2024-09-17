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

criterion::CostVector CostAggregator::getCost(
    const model::DesignBuilder &design) const {
  auto designBuilder = const_cast<model::DesignBuilder&>(design);
  auto designCost = criterion::CostVector::Zero;

  for (size_t i = 0; i < design.getSubnetNum(); ++i) {
    const auto subnetBuilder = designBuilder.getSubnetBuilder(i);
    const auto subnetCost = subnetEstimator.getCost(*subnetBuilder);

    criterion::aggregateCost(designCost, subnetCost);
  }

  return designCost;
}

} // namespace eda::gate::estimator
