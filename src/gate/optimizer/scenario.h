//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/design.h"
#include "gate/model/subnet.h"
#include "gate/optimizer/transformer.h"

#include <functional>
#include <memory>
#include <string>

namespace eda::gate::optimizer {

/**
 * @brief Base class for a scenario state.
 */
template <typename Builder>
struct ScenarioState {
  ScenarioState(const BuilderPtr<Builder> &builder): builder(builder) {}
  virtual ~ScenarioState() {}

  const BuilderPtr<Builder> &builder;
}; 

/**
 * @brief Interface for subnet/design optimization scenario.
 */
template <typename Builder>
class Scenario {
public:
  using State = ScenarioState<Builder>;
  using Action = std::shared_ptr<InPlaceTransformer<Builder>>;

  Scenario(const std::string &name): name(name) {}
  virtual ~Scenario() {}

  /// Returns the scenario name.
  std::string getName() const { return name; }

  /// Returns the initial scenario state.
  virtual std::unique_ptr<State> initialize(
      const BuilderPtr<Builder> &builder) const = 0;

  /// Checks whether the scenario is over.
  virtual bool isOver(State &state) const = 0;

  /// Returns an optimization pass to be applied to the builder.
  virtual Action getAction(State &state) const = 0;

  /// Finalizes the sceanario.
  virtual void finalize(State &state) const = 0;

private:
  const std::string name;
};

/**
 * @brief Executes an optimization scenario.
 */
template <typename Builder>
struct ScenarioExecutor final : public InPlaceTransformer<Builder> {
  using State = typename Scenario<Builder>::State;
  using Action = typename Scenario<Builder>::Action;
  using Callback = std::function<void(const State &, const Action &)>;

  ScenarioExecutor(const std::string &name,
                   const Scenario<Builder> &scenario,
                   const Callback *onBegin = nullptr,
                   const Callback *onEnd = nullptr):
      InPlaceTransformer<Builder>(name),
      scenario(scenario),
      onBegin(onBegin),
      onEnd(onEnd) {}

  ScenarioExecutor(const Scenario<Builder> &scenario,
                   const Callback *onBegin = nullptr,
                   const Callback *onEnd = nullptr):
      ScenarioExecutor(scenario.getName(), scenario, onBegin, onEnd) {}

  void setMaxLength(const size_t maxLength) {
    this->maxLength = maxLength;
  }

  void transform(const BuilderPtr<Builder> &builder) const override {
    auto state = scenario.initialize(builder);
    for (size_t i = 0; i < maxLength && !scenario.isOver(*state); ++i) {
      const auto action = scenario.getAction(*state);
      if (!action) break;

      if (onBegin) (*onBegin)(*state, action);
      action->transform(builder);
      if (onEnd) (*onEnd)(*state, action);
    }
    scenario.finalize(*state);
  }

private:
  const Scenario<Builder> &scenario;

  const Callback *onBegin;
  const Callback *onEnd;

  size_t maxLength{-1ull};
};

using SubnetScenarioState = ScenarioState<model::SubnetBuilder>;
using DesignScenarioState = ScenarioState<model::DesignBuilder>;

using SubnetScenario = Scenario<model::SubnetBuilder>;
using DesignScenario = Scenario<model::DesignBuilder>;

using SubnetScenarioExecutor = ScenarioExecutor<model::SubnetBuilder>;
using DesignScenarioExecutor = ScenarioExecutor<model::DesignBuilder>;

} // namespace eda::gate::optimizer
