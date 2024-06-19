//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/criterion/criterion.h"

#include <cassert>
#include <cmath>
#include <set>

namespace eda::gate::optimizer {

static inline std::pair<float, float> getProgressRange(float progress) {
  constexpr float t{2.0};
  return {std::pow(progress, t), std::pow(progress, 1/t)};
}

static inline CostVector predictCostVector(
    const CostVector &vector, const float progress) {
  constexpr float e{1.0e-6};
  return progress > e ? vector / progress : CostVector::Zero;
}

template <typename T>
class SolutionSpace final {
public:
  struct Solution final {
    Solution(const T &solution, const Cost cost, const CostVector &vector):
      solution(solution), cost(cost), vector(vector) {}

    const T solution;
    const Cost cost;
    const CostVector vector;

    bool operator==(const Solution &other) const {
      return solution == other.solution;
    }

    bool operator<(const Solution &other) const {
      return cost < other.cost;
    }

    bool operator>(const Solution &other) const {
      return cost > other.cost;
    }
  };

  using Region = std::set<Solution>;

  SolutionSpace(const Criterion &criterion,
                const CostVector &tension,
                const float progress):
      criterion(criterion),
      tension(tension),
      progress(progress) {}

  bool getCost(const CostVector &vector) const {
    return criterion.getCost(vector);
  }

  Cost getPenalty(const CostVector &vector) const {
    const auto prediction = predictCostVector(vector, progress);
    return criterion.getPenalty(prediction, tension);
  }

  Cost getPenalizedCost(const CostVector &vector) const {
    return getCost(vector) * getPenalty(vector);
  }

  CostVector getTension(const CostVector &vector) const {
    const auto maxProgress = getProgressRange(progress).second;
    const auto minPrediction = predictCostVector(vector, maxProgress);
    return criterion.getTension(minPrediction);
  }

  CostVector getTension() const {
    assert(hasSolution());
    return getTension(getBest().vector);
  }

  bool check(const CostVector &vector) const {
    const auto maxProgress = getProgressRange(progress).second;
    const auto minPrediction = predictCostVector(vector, maxProgress);
    return criterion.check(minPrediction);
  }

  /// Add the solution.
  void add(const T &solution, const CostVector &vector) {
    const auto cost = getPenalizedCost(vector);
    feasible |= check(vector);
    solutions.insert(Solution{solution, cost, vector});
  }

  /// Checks whether there are solutions.
  bool hasSolution() const { return !solutions.empty(); }
  /// Checks whether there are feasible solutions. 
  bool hasFeasible() const { return feasible; }

  /// Returns the best solution w.r.t. to the given criterion.
  const Solution &getBest() const { return *solutions.begin(); }

private:
  const Criterion &criterion;
  const CostVector tension;
  const float progress;

  bool feasible{false};
  Region solutions;
};

} // namespace eda::gate::optimizer
