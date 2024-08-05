//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/criterion/cost_function.h"
#include "gate/model/design.h"
#include "gate/model/subnet.h"

namespace eda::gate::estimator {

/**
 * @brief Interface for design/subnet cost estimator.
 */
class CostEstimator {
public:
  virtual criterion::CostVector getCost(const model::SubnetBuilder &subnet) = 0;
  virtual criterion::CostVector getCost(const model::DesignBuilder &design) = 0;

  virtual ~CostEstimator() {}
};

} // namespace eda::gate::estimator
