//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/estimator/switching_activity.h"
#include "gate/simulator/simulator.h"
#include "util/assert.h"

#include <bitset>
#include <random>
#include <tuple>
#include <utility>
#include <vector>

namespace eda::gate::estimator {

/**
 * @brief Evaluates switching activity by counting switches using simulation.
 */
class SimulationEstimator final : public SwitchActivityEstimator {
public:

  /// @cond ALIASES
  using Builder       = model::SubnetBuilder;
  using Cache         = simulator::Simulator::DataChunk;
  using CacheList     = simulator::Simulator::DataVector;
  using Distributions = std::vector<std::discrete_distribution<int>>;
  using InValuesList  = std::vector<CacheList>;
  using OnStates      = SwitchActivity::Probabilities;
  using Probabilities = SwitchActivity::Probabilities;
  using Simulator     = simulator::Simulator;
  using Switches      = SwitchActivity::Switches;
  /// @endcond

  /**
   * @brief Creates simulator-based estimator.
   * @param ticks The number of ticks for subnet simulation.
   */
  explicit SimulationEstimator(size_t ticks = 1024) :
      generator((std::random_device())()) {
    setTicks(ticks);
  }

  /**
   * @brief Sets the number of simulation ticks.
   * @param newTicks The number of ticks for subnet simulation.
   */
  void setTicks(size_t newTicks) {

    uassert(newTicks, "The number of ticks cannot be zero");
    
    const size_t simulations = std::ceil(newTicks * 1. / simCacheSize);
    this->simulationCount = simulations;
    this->ticks = (simCacheSize * simulations);
  }

  /**
   * Estimates the switching activity by calculating the number of
   * transitions from 0 to 1 and from 1 to 0 of each cell during simulation:
   * 
   *  SwitchActivity[i] = tc[i] / (n - 1),
   *  
   *  i  - id of the cell,
   *  tc - the number of 0->1 and 0->1 switches for the cell,
   *  n  - the number of the ticks.
   *  
   * @param builder The builder with Subnet for estimating.
   * @param probabilities The probabilities of getting 1 for each input in
   * the subnet. If it is not passed, the probability of getting 1
   * for each input is 0.5.
   * @param result The switching activity of the subnet.
   */
  void estimate(const std::shared_ptr<Builder> &builder,
                const Probabilities &probabilities,
                SwitchActivity &result) const override;

  /**
   * @brief Simulates the subnet and stores information about cells states.
   * @param subnet The subnet for simulation.
   * @param inValuesList The input values for simulation of the subnet.
   * @return Switches from 1 to 0 and from 0 to 1 for the subnet and count
   * of on state for cells.
   */
  std::tuple<Switches, Switches, OnStates> simulate(
      const Builder &builder,
      const InValuesList &inValuesList) const;

private:

  Cache generateInValues(Distributions &distributions, size_t id) const;

  static uint8_t popCount(uint64_t number) {
    return std::bitset<64>(number).count();
  }

  /**
   * @brief Returns all switched bits in cache (last bit can not switch).
   */
  static uint64_t getSwitchedBits(uint64_t cache) {
    return (cache ^ (cache >> 1)) & ~(1ull << 63);
  }

  /// The size of simuation cache for a cell.
  static const size_t simCacheSize{64};
  /// The number of simulation ticks.
  size_t ticks{1024};
  /// The simulations count (1 sumulation == 64 ticks).
  size_t simulationCount;
  /// Random numbers generator.
  mutable std::mt19937 generator;
};

} // namespace eda::gate::estimator
