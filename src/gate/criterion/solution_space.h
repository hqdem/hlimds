//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/criterion/criterion.h"

#include <cassert>
#include <cmath>
#include <type_traits>

namespace eda::gate::criterion {

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
  static_assert(std::is_copy_assignable_v<T>, "T must be copy-assignable");

public:
  struct Solution final {
    Solution():
      cost(std::numeric_limits<Cost>::max()) {}
    Solution(const T &solution, const Cost cost, const CostVector &vector):
      solution(solution), cost(cost), vector(vector) {}
    Solution(const Solution &other) = default;

    bool operator==(const Solution &other) const {
      return solution == other.solution;
    }

    bool operator<(const Solution &other) const {
      return cost < other.cost;
    }

    bool operator>(const Solution &other) const {
      return cost > other.cost;
    }

    Solution &operator=(const Solution &other) = default;

    T solution;
    Cost cost;
    CostVector vector;
  };

  SolutionSpace(const Criterion &criterion,
                const CostVector &tension,
                const float progress):
      criterion(criterion),
      tension(tension),
      progress(progress) {}

  Cost getCost(const CostVector &vector) const {
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
    const auto isFeasible = check(vector);  

    if (feasibleCount == 0 || cost < best.cost) {
      best = Solution(solution, cost, vector);
    }

    solutionCount++;
    feasibleCount += isFeasible;
  }

  /// Checks whether there are solutions.
  bool hasSolution() const { return solutionCount > 0; }
  /// Checks whether there are feasible solutions. 
  bool hasFeasible() const { return feasibleCount > 0; }

  /// Returns the best solution w.r.t. to the given criterion.
  const Solution &getBest() const { return best; }

private:
  const Criterion &criterion;
  const CostVector tension;
  const float progress;

  size_t solutionCount{0};
  size_t feasibleCount{0};

  Solution best;
};

} // namespace eda::gate::criterion
