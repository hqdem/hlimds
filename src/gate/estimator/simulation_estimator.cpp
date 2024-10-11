//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/estimator/simulation_estimator.h"

#include <kitty/kitty.hpp>

#include <bitset>

namespace eda::gate::estimator {

using OnStates = SimulationEstimator::OnStates;
using Switches = SimulationEstimator::Switches;

struct PrevBits {

  PrevBits(size_t n) : _bits(n > 63 ? std::ceil(n * 1. / 64.) : 1) { }

  uint64_t get(size_t id) const {
    return kitty::get_bit(*this, id);
  };

  void set(size_t id) {
    kitty::set_bit(*this, id);
  };

  void clear(size_t id) {
    kitty::clear_bit(*this, id);
  };

  /// @brief Prefix "_" is needed for using methods from kitty.
  std::vector<uint64_t> _bits;
};

void SimulationEstimator::estimate(const std::shared_ptr<Builder> &builder,
                                   const Probabilities &probabilities,
                                   SwitchActivity &result) const {

  InValuesList inValuesList;
  inValuesList.reserve(simulationCount);

  Distributions distributions;

  for (double prob : probabilities) {
    distributions.push_back({1 - prob, prob});
  }

  for (size_t i{0}; i < simulationCount; ++i) {
    CacheList cacheList;
    for (size_t id{0}; id < builder->getInNum(); ++id) {
      cacheList.push_back(generateInValues(distributions, id));
    }
    inValuesList.push_back(std::move(cacheList));
  }

  auto [on, off, onStates] = simulate(builder, inValuesList);

  Probabilities switching(on.size());
  for (size_t id{0}; id < on.size(); ++id) {
    switching[id] = static_cast<double>(on[id] + off[id]) / (ticks - 1);
    onStates[id] /= ticks;
  }

  result.setSwitchActivity(switching, onStates, on, off, ticks);
}

std::tuple<Switches, Switches, OnStates> SimulationEstimator::simulate(
    const std::shared_ptr<Builder> &builder,
    const InValuesList &inValuesList) const {

  Simulator simulator(builder);

  const size_t inputs = builder->getInNum();
  const auto size = builder->getMaxIdx() + 1;

  Switches switchesOn(size);
  Switches switchesOff(size);
  OnStates onStates(size);

  PrevBits prev(size);

  for (size_t i{0}; i < inValuesList.size(); ++i) {
    const CacheList &values = inValuesList[i];

    uassert(values.size() == inputs,
        "The number of input values is not equal to the number of inputs");

    simulator.simulate(values);

    for (auto it = builder->begin(); it != builder->end(); it.nextCell()) {
      const auto id = *it;
      Cache cache = simulator.getValue(id);
      onStates[id] += popCount(cache) * 1.f;
      uint64_t bits = getSwitchedBits(cache);
      uint64_t prevBit = ((i) && ((cache & 1ull) ^ prev.get(id)));
      switchesOn[id] += popCount(bits & ~cache) + (prevBit & cache);
      switchesOff[id] += popCount(bits & cache) + (prevBit & ~cache);
      (cache & (1ull << 63)) ? prev.set(id) : prev.clear(id);
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

} // namespace eda::gate::estimator
