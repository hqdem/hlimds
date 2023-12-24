//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"

#include <utility>
#include <vector>

namespace eda::gate::analyzer {

/**
 * @brief Stores information about the switching activity of subnet cells.
 */
class SwitchActivity final {
public:

  using CellActivities = std::vector<double>;

  explicit SwitchActivity(const CellActivities &cellActivities)
      : cellActivities(cellActivities) { }

  explicit SwitchActivity(CellActivities &&cellActivities)
      : cellActivities(cellActivities) { }

  const CellActivities &getCellActivities() const {
    return cellActivities;
  }

  /**
   * Returns the sum of the activities of all cells.
   */
  double getActivitySum() const {
    double result{0.0};
    for (double activity : cellActivities) {
      result += activity;
    }
    return result;
  }

private:
  // Сontains the switching activity of cells (accessed via cell indices).
  CellActivities cellActivities;
};

/**
 * @brief Implements an interface for switching activity estimators.
 */
class SwitchActivityEstimator {
public:

  using Probabilities = std::vector<double>;
  using Subnet        = model::Subnet;

  /**
   * @brief Estimates the switching activity of each cell in the subnet.
   * @param subnet        The subnet for estimating.
   * @param probabilities The probabilities of getting 1 for each input in
   * the subnet. If it is not passed, the probability of getting 1
   * for each input is 0.5.
   */
  virtual SwitchActivity estimate(const Subnet &subnet,
      const Probabilities &probabilities = {}) = 0;

  virtual ~SwitchActivityEstimator() = default;
};

} // namespace eda::gate::analyzer
