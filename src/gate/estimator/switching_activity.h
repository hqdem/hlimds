//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/estimator/estimator.h"
#include "gate/model/subnet.h"

#include <ostream>
#include <utility>
#include <vector>

namespace eda::gate::estimator {

/**
 * @brief Stores information about the switching activity of subnet cells.
 */
class SwitchActivity final {
public:

  /// Probabilities for cells activity description.
  using Probabilities = std::vector<float>;
  /// The switches of cells.
  using Switches = std::vector<size_t>;

  SwitchActivity() = default;

  /**
   * Constructor from cells switching and on state probabilities
   * and switches of cells.
   * @param switching Сells switching probabilities.
   * @param onState Сells on state probabilities.
   * @param on The switches of cells from 0 to 1.
   * @param off The switches of cells from 1 to 0.
   */
  SwitchActivity(Probabilities &&switching, Probabilities &&onState,
                 Switches &&on, Switches &&off, size_t ticks)
      : switchProbabilities(std::move(switching)),
        onStateProbabilities(std::move(onState)), 
        switchesOn(std::move(on)), switchesOff(std::move(off)), ticks(ticks) { }

  /**
   * Constructor from cells switching and on state probabilities.
   * @param switching Сells switching probabilities.
   * @param onState Сells on state probabilities.
   */
  SwitchActivity(Probabilities &&switching, Probabilities &&onState)
      : switchProbabilities(std::move(switching)),
        onStateProbabilities(std::move(onState)) { }

  /**
   * @brief Returns the sum of the switching probabilities of all cells.
   */
  double getSwitchProbsSum() const {
    double result{0.0};
    for (float prob : switchProbabilities) {
      result += prob;
    }
    return result;
  }

  /**
   * @brief Returns cell switching probability.
   */
  float getSwitchProbability(size_t id) const {
    assert(switchProbabilities.size() > id);
    return switchProbabilities[id];
  }

  /**
   * @brief Returns cell on state probability.
   */
  float getOnStateProbability(size_t id) const {
    assert(onStateProbabilities.size() > id);
    return onStateProbabilities[id];
  }

  /**
   * @brief Returns cell switches from 0 to 1.
   */
  size_t getSwitchesOn(size_t id) const {
    assert(switchesOn.size() > id);
    return switchesOn[id];
  }

  /**
   * @brief Returns switches from 1 to 0.
   */
  size_t getSwitchesOff(size_t id) const {
    assert(switchesOff.size() > id);
    return switchesOff[id];
  }

  /**
   * @brief Returns the number of simulation ticks during estimation.
   */
  size_t getTicks() const {
    return ticks;
  }

  void setSwitchActivity(Probabilities &switching, Probabilities &onState,
                         Switches &on, Switches &off, size_t ts) {

    setSwitchActivity(switching, onState);
    std::swap(switchesOn, on);
    std::swap(switchesOff, off);
    ticks = ts;
  }

  void setSwitchActivity(Probabilities &switching, Probabilities &onState) {
    std::swap(switchProbabilities, switching);
    std::swap(onStateProbabilities, onState);
  }

private:
  /// Сontains the switching probabilities of cells (accessed via cell indices).
  Probabilities switchProbabilities;
  /// Сontains the on state probabilities of cells (accessed via cell indices).
  Probabilities onStateProbabilities;
  /// Switches from 0 to 1.
  Switches switchesOn;
  /// Switches from 1 to 0.
  Switches switchesOff;
  /// Ticks of simulations (for simulation estimator).
  size_t ticks{0};
};

/// @brief Implements an interface for switching activity estimators.
using SwitchActivityEstimator = SubnetEstimator<SwitchActivity::Probabilities,
                                                SwitchActivity>;

/**
 * @brief Print information about switching activity of the subnet.
 * @param switchActivity The switching activity of the subnet.
 * @param subnet The subnet for estimating.
 * @param out output stream.
 */
void printSwitchActivity(const SwitchActivity &switchActivity,
                         const model::SubnetBuilder &builder,
                         std::ostream &out);

} // namespace eda::gate::estimator
