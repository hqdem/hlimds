//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"

#include <utility>
#include <vector>

namespace eda::gate::analyzer {

/**
 * @brief Stores information about the switching activity of subnet cells.
 */
class SwitchActivity final {
public:

  /// Сells switching probabilities.
  using Probabilities = std::vector<double>;
  /// The switches of cells.
  using Switches = std::vector<size_t>;

  /**
   * @brief Constructor from cell switching probabilities and switches of cells.
   * @param probabilities Сell switching probabilities.
   * @param on The switches of cells from one to zero.
   * @param off The switches of cells from zero to one.
   */
  SwitchActivity(Probabilities &&probabilities, Switches &&on, Switches &&off)
      : probabilities(probabilities), switchesOn(on), switchesOff(off) { }

  /**
   * @brief Constructor from cells switching probabilities.
   * @param probabilities Сells switching probabilities.
   */
  explicit SwitchActivity(Probabilities &&probabilities)
      : probabilities(probabilities), switchesOn(), switchesOff() { }

  /**
   * @brief Returns the sum of the activities of all cells.
   */
  double getActivitySum() const {
    double result{0.0};
    for (double activity : probabilities) {
      result += activity;
    }
    return result;
  }

  /**
   * @brief Returns cells switching probabilities.
   */
  const Probabilities &getActivities() const {
    return probabilities;
  }

  /**
   * @brief Returns switches from 0 to 1.
   */
  const Switches &getSwitchesOn() const {
    return switchesOn;
  }

  /**
   * @brief Returns switches from 1 to 0.
   */
  const Switches &getSwitchesOff() const {
    return switchesOff;
  }  

private:
  /// Сontains the switching activity of cells (accessed via cell indices).
  Probabilities probabilities;
  /// Switches from one to zero.
  Switches switchesOn;
  /// Switches from zero to one.
  Switches switchesOff;
};

/**
 * @brief Implements an interface for switching activity estimators.
 */
class SwitchActivityEstimator {
public:

  /// @cond ALIASES
  using Probabilities = SwitchActivity::Probabilities;
  using Subnet        = model::Subnet;
  /// @endcond

  /**
   * @brief Estimates the switching activity of each cell in the subnet.
   * @param subnet The subnet for estimating.
   * @param inputProbabilities The probabilities of getting 1 for each input in
   * the subnet. If it is not passed, the probability of getting 1
   * for each input is 0.5.
   */
  virtual SwitchActivity estimate(const Subnet &subnet,
      const Probabilities &inputProbabilities = {}) = 0;

  virtual ~SwitchActivityEstimator() = default;
};

} // namespace eda::gate::analyzer
