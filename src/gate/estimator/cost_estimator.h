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
template <typename T>
struct CostEstimator {
  virtual criterion::CostVector getCost(const T &design) const = 0;
  virtual ~CostEstimator() {}
};

using SubnetEstimator = CostEstimator<model::SubnetBuilder>;
using DesignEstimator = CostEstimator<model::DesignBuilder>;

/**
 * @brief Aggregates subnet cost vectors.
 */
class CostAggregator : public DesignEstimator {
public:
  CostAggregator(const SubnetEstimator &subnetEstimator):
      subnetEstimator(subnetEstimator) {}

  criterion::CostVector getCost(
      const model::DesignBuilder &design) const override;

private:
  const SubnetEstimator &subnetEstimator;
};

/**
 * @brief Returns logical characteristics of a subnet:
 *        the number of cells as AREA;
 *        the depth as DELAY;
 *        the switching activity as POWER.
 */
struct LogicSubnetEstimator final : public SubnetEstimator {
  criterion::CostVector getCost(
      const model::SubnetBuilder &subnet) const override;
};

/**
 * @brief Returns logical characteristics of a design.
 */
class LogicDesignEstimator final : public CostAggregator {
public:
  LogicDesignEstimator(): CostAggregator(subnetEstimator) {}

private:
  const LogicSubnetEstimator subnetEstimator{};
};

} // namespace eda::gate::estimator
