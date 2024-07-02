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

using OnStates = SimulationEstimator::OnStates;
using Switches = SimulationEstimator::Switches;

SwitchActivity SimulationEstimator::estimate(const Subnet &subnet,
    const Probabilities &probabilities) const {

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

  auto [switchesOn, switchesOff, onStates] = simulate(subnet, inValuesList);
  
  Probabilities switching(switchesOn.size());
  for (size_t id{0}; id < switchesOn.size(); ++id) {
    switching[id] = static_cast<double>(switchesOn[id] + switchesOff[id]) /
        (ticks - 1);
    onStates[id] /= ticks; 
  }

  return SwitchActivity{std::move(switching), std::move(onStates),
      std::move(switchesOn), std::move(switchesOff), ticks};
}

// FIXME: change Subnet to SubnetBuilder.
std::tuple<Switches, Switches, OnStates> SimulationEstimator::simulate(
    const Subnet &subnet, const InValuesList &inValuesList) const {

  model::SubnetBuilder builder(subnet); // FIXME:
  Simulator simulator(builder);

  const size_t inputs = subnet.getInNum(); // FIXME:
  const auto cells = subnet.getEntries(); // FIXME:

  uassert(cells.size() - inputs, "Subnet has only inputs");

  Switches switchesOn(cells.size());
  Switches switchesOff(cells.size());
  OnStates onStates(cells.size());

  size_t ttSize = std::ceil(std::log2(cells.size()));
  kitty::dynamic_truth_table prevBits(ttSize ? ttSize : 1);

  auto getPrevBit = [&prevBits](size_t id) {
    return kitty::get_bit(prevBits, id);
  };

  auto setPrevBit = [&prevBits](size_t id) {
    return kitty::set_bit(prevBits, id);
  };

  auto reSetPrevBit = [&prevBits](size_t id) {
    return kitty::clear_bit(prevBits, id);
  };

  for (size_t i{0}; i < inValuesList.size(); ++i) {
    const CacheList &values = inValuesList[i];

    uassert(values.size() == inputs,
        "The number of input values is not equal to the number of inputs");

    simulator.simulate(values);

    for (auto it = builder.begin(); it != builder.end(); it.nextCell()) {
    //FIXME: for (size_t id{0}; id < cells.size(); ++id) {
      const auto id = *it;
      Cache cache = simulator.getValue(id);
      onStates[id] += popCount(cache) * 1.f;
      uint64_t bits = getSwitchedBits(cache);
      uint64_t prevBit = ((i) && ((cache & 1ull) ^ getPrevBit(id)));
      switchesOn[id] += popCount(bits & ~cache) + (prevBit & cache);
      switchesOff[id] += popCount(bits & cache) + (prevBit & ~cache);
      (cache & (1ull << 63)) ? setPrevBit(id) : reSetPrevBit(id);
    //FIXME:  id += cells[id].cell.more;
    }
  }

  return {std::move(switchesOn), std::move(switchesOff), std::move(onStates)};
}

SimulationEstimator::Cache SimulationEstimator::generateInValues(
    Distributions &distributions, size_t id) const {
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
