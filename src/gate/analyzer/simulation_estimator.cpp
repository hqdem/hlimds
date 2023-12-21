//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/analyzer/simulation_estimator.h"

#include <kitty/kitty.hpp>

#include <bitset>

namespace eda::gate::analyzer {

SwitchActivity SimulationEstimator::estimate(const Subnet &subnet,
    const Probabilities &probabilities) {

  const size_t sizeOfSimulationCache{64};
  const size_t simulationCount = std::ceil(ticks / sizeOfSimulationCache);

  InValuesList inValuesList;
  inValuesList.reserve(simulationCount);

  Distributions distributions;

  for (double prob : probabilities) {
    distributions.push_back({1 - prob, prob});
  }

  for (size_t i{0}; i < simulationCount; ++i) {
    CacheList cacheList;
    for (size_t id{0}; id < subnet.getInNum(); ++id) {
      cacheList.push_back(generateInValues(distributions, id));
    }
    inValuesList.push_back(std::move(cacheList));
  }

  Switches switches = countSwitches(subnet, inValuesList);
  
  SwitchActivity::CellActivities result(switches.size());
  for (size_t id{0}; id < switches.size(); ++id) {
    result[id] = static_cast<double>(switches[id]) / (ticks - 1);
  }

  return SwitchActivity{std::move(result)};
}

SimulationEstimator::Switches SimulationEstimator::countSwitches(
    const Subnet &subnet, const InValuesList &inValuesList) {

  const size_t inputs = subnet.getInNum();
  const auto cells = subnet.getEntries();

  uassert(cells.size() - inputs, "Subnet has only inputs");

  Simulator simulator(subnet);

  Switches result(cells.size());

  size_t ttSize = std::ceil(std::log2(cells.size()));
  kitty::dynamic_truth_table lastBits(ttSize ? ttSize : 1);

  auto getLastBit = [&lastBits](size_t id) {
    return kitty::get_bit(lastBits, id);
  };

  auto setLastBit = [&lastBits](size_t id) {
    return kitty::set_bit(lastBits, id);
  };

  auto reSetLastBit = [&lastBits](size_t id) {
    return kitty::clear_bit(lastBits, id);
  };

  for (size_t i{0}; i < inValuesList.size(); ++i) {

    const CacheList &values = inValuesList[i];

    uassert(values.size() == inputs,
        "The number of input values is not equal to the number of inputs");

    simulator.simulate(values);

    for (size_t id{0}; id < cells.size(); ++id) {
      Cache cache = simulator.getValue(id);
      result[id] += popCount(getToggledBits(cache));
      result[id] += (i) && ((cache & 1ull) ^ getLastBit(id)) ? 1 : 0;
      (cache & (1ull << 63)) ? setLastBit(id) : reSetLastBit(id);
      id += cells[id].cell.more;
    }
  }

  return result;
}

SimulationEstimator::Cache SimulationEstimator::generateInValues(
    Distributions &distributions, size_t id) {
  if (distributions.empty()) {
    return static_cast<uint64_t>(std::rand()) +
        (static_cast<uint64_t>(std::rand()) << 32);
  }
  Cache bits{0};
  auto &dist = distributions[id];
  for (size_t bit{0}; bit < 64; ++bit) {
    bits |= (static_cast<uint64_t>(dist(generator)) << bit);
  }
  return bits;
}

} // namespace eda::gate::analyzer
