//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/analyzer/switching_activity.h"
#include "gate/simulator2/simulator.h"
#include "util/assert.h"

#include <bitset>
#include <random>
#include <vector>

namespace eda::gate::analyzer {

/**
 * @brief Evaluates switching activity by counting switches using simulation.
 */
class SimulationEstimator final : public SwitchActivityEstimator {
public:

  using Cache         = simulator2::Simulator::DataChunk;
  using CacheList     = simulator2::Simulator::DataVector;
  using Distributions = std::vector<std::discrete_distribution<int>>;
  using InValuesList  = std::vector<CacheList>;
  using Simulator     = simulator2::Simulator;
  using Switches      = std::vector<size_t>;

  /**
   * @brief Creates simulator-based estimator.
   * 
   * @param ticks The number of ticks for subnet simulation.
   */
  explicit SimulationEstimator(size_t ticks = 1024) : ticks(ticks),
      generator((std::random_device())()) { }

  /**
   * Sets the number of simulation ticks.
   * 
   * @param ticks The number of ticks for subnet simulation.
   */
  void setTicks(size_t newTicks) {

    uassert(newTicks, "The number of ticks cannot be zero");

    this->ticks = newTicks;
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
   * @param subnet        The subnet for estimating.
   * @param probabilities The probabilities of getting 1 for each input in
   * the subnet. If it is not passed, the probability of getting 1
   * for each input is 0.5.
   */
  SwitchActivity estimate(const Subnet &subnet,
                          const Probabilities &probabilities = {}) override;

  /**
   * Сounts only the switches from 0 to 1 and from 1 to 0 for
   * passed input values.
   * 
   * @param subnet       The subnet for counting switches.
   * @param inValuesList The input values for simulation of the subnet.
   */
  Switches countSwitches(const Subnet &subnet,
                         const InValuesList &inValuesList);

private:

  Cache generateInValues(Distributions &distributions, size_t id);  

  static uint8_t popCount(uint64_t number) {
    return std::bitset<64>(number).count();
  }

  /**
   * Returns all switched bits in cache (last bit can not switch).
   */
  static uint64_t getToggledBits(uint64_t cache) {
    return (cache ^ (cache >> 1)) & ~(1ull << 63);
  }

  // TODO: Decide about default value for the ticks.
  size_t ticks{1024};
  std::mt19937 generator;
};

} // namespace eda::gate::analyzer
