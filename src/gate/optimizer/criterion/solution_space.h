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
#include <set>

namespace eda::gate::optimizer {

template <typename T>
class SolutionSpace final {
public:
  struct Solution final {
    Solution(const T &solution, const Cost cost, const CostVector &vector):
      solution(solution), cost(cost), vector(vector) {}

    const T solution;
    const Cost cost;
    const CostVector vector;

    bool operator==(const Solution &rhs) const {
      return solution == rhs.solution;
    }

    bool operator<(const Solution &rhs) const {
      return cost < rhs.cost;
    }

    bool operator>(const Solution &rhs) const {
      return cost > rhs.cost;
    }
  };

  using Region = std::set<Solution>;

  SolutionSpace(const Criterion &criterion): criterion(criterion) {}

  /// Add the solution.
  void add(const T &solution, const CostVector &vector) {
    Region &region = criterion.check(vector) ? feasible : infeasible;
    region.insert(Solution{solution, criterion.cost(vector), vector});
  }

  /// Checks whether there are feasible solutions. 
  bool hasFeasible() const { return !feasible.empty(); }

  /// Returns the best solution w.r.t. to the given criterion.
  const Solution &getBest() const { return *feasible.begin(); }

private:
  const Criterion &criterion;

  Region feasible;
  Region infeasible;
};

} // namespace eda::gate::optimizer
