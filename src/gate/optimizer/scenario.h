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
  Scenario(const std::string &name): name(name) {}
  virtual ~Scenario() {}

  /// Returns the scenario name.
  std::string getName() const { return name; }

  /// Returns the initial scenario state.
  virtual std::unique_ptr<ScenarioState<Builder>> initialize(
      const BuilderPtr<Builder> &builder) const = 0;

  /// Checks whether the scenario is over.
  virtual bool isOver(ScenarioState<Builder> &state) const = 0;

  /// Applies an optimization pass to the builder.
  virtual void transform(ScenarioState<Builder> &state) const = 0;

  /// Finalizes the sceanario.
  virtual void finalize(ScenarioState<Builder> &state) const = 0;

private:
  const std::string name;
};

/**
 * @brief Executes an optimization scenario.
 */
template <typename ID, typename Builder>
struct ScenarioExecutor final : public InPlaceTransformer<ID, Builder> {
  ScenarioExecutor(const std::string &name, const Scenario<Builder> &scenario):
      InPlaceTransformer<ID, Builder>(name), scenario(scenario) {}

  ScenarioExecutor(const Scenario<Builder> &scenario):
      ScenarioExecutor(scenario.getName(), scenario) {}

  void setMaxLength(const size_t maxLength) {
    this->maxLength = maxLength;
  }

  void transform(const BuilderPtr<Builder> &builder) const override {
    auto state = scenario.initialize(builder);
    for (size_t i = 0; i < maxLength && !scenario.isOver(state); ++i) {
      scenario.transform(state);
    }
    scenario.finalize(state);
  }

private:
  const Scenario<Builder> &scenario;
  size_t maxLength{-1ull};
};

using SubnetScenarioState = ScenarioState<model::SubnetBuilder>;
using DesignScenarioState = ScenarioState<model::DesignBuilder>;

using SubnetScenario = Scenario<model::SubnetBuilder>;
using DesignScenario = Scenario<model::DesignBuilder>;

using SubnetScenarioExecutor =
    ScenarioExecutor<model::SubnetID, model::SubnetBuilder>;
using DesignScenarioExecutor =
    ScenarioExecutor<model::NetID, model::DesignBuilder>;

} // namespace eda::gate::optimizer
