//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/criterion/criterion.h"
#include "gate/criterion/solution_space.h"
#include "gate/model/subnet.h"
#include "gate/optimizer/cut.h"
#include "gate/optimizer/transformer.h"

#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace eda::gate::techmapper {

/**
 * @brief General dynamic programming-based subnet mapper.
 */
class SubnetTechMapper final : public optimizer::SubnetTransformer {
public:
  struct Match final {
    model::CellTypeID typeID{model::OBJ_NULL_ID};
    model::Subnet::LinkList links{};
    bool inversion{false};
  };

  struct Context final {};

  using CutProvider =
      std::function<optimizer::CutsList(
                        const model::SubnetBuilder &,
                        const uint32_t entryID)>;
  using MatchFinder =
      std::function<std::vector<Match>(
                        const model::SubnetBuilder &,
                        const optimizer::Cut &)>;
  using CellEstimator =
      std::function<criterion::CostVector(
                        const model::CellTypeID,
                        const Context &)>;
  using CostAggregator =
      std::function<criterion::CostVector(
                        const std::vector<criterion::CostVector> &)>;
  using CostPropagator =
      std::function<criterion::CostVector(
                        const criterion::CostVector &,
                        const uint32_t fanout)>;

  using CellSpace = criterion::SolutionSpace<Match>;
  using SubnetSpace = std::vector<std::unique_ptr<CellSpace>>;

  SubnetTechMapper(const std::string &name,
                   const criterion::Criterion &criterion,
                   const CutProvider cutProvider,
                   const MatchFinder matchFinder,
                   const CellEstimator cellEstimator,
                   const CostAggregator costAggregator,
                   const CostPropagator costPropagator):
      optimizer::SubnetTransformer(name),
      criterion(criterion),
      cutProvider(cutProvider),
      matchFinder(matchFinder),
      cellEstimator(cellEstimator),
      costAggregator(costAggregator),
      costPropagator(costPropagator) {}

  SubnetTechMapper(const std::string &name,
                   const criterion::Criterion &criterion,
                   const CutProvider cutProvider,
                   const MatchFinder matchFinder,
                   const CellEstimator cellEstimator);

  std::shared_ptr<model::SubnetBuilder> map(
      const std::shared_ptr<model::SubnetBuilder> &builder) const override;

private:
  struct Status final {
    enum Verdict {
      /// Solution is found (tension shows how good it is).
      FOUND,
      /// Solution does not exist (no matching).
      UNSAT,
      /// Early recovery (before finding a solution).
      RERUN
    };

    Status() = default;

    Status(const Verdict verdict): verdict(verdict) {}

    Status(const Verdict verdict,
           const bool isFeasible,
           const float progress,
           const criterion::CostVector &vector,
           const criterion::CostVector &tension):
        verdict(verdict),
        isFeasible(isFeasible),
        vector(vector),
        tension(tension) {}

    Status(const Verdict verdict,
           const float progress,
           const criterion::CostVector &vector,
           const criterion::CostVector &tension):
        Status(verdict, false, progress, vector, tension) {}

    Status(const Verdict verdict,
           const bool isFeasible,
           const criterion::CostVector &vector,
           const criterion::CostVector &tension):
        Status(verdict, isFeasible, 1., vector, tension) {}

    Verdict verdict{UNSAT};
    bool isFeasible{false};
    float progress{1.0};
    criterion::CostVector vector;
    criterion::CostVector tension;
  };

  Status map(
      const std::shared_ptr<model::SubnetBuilder> &builder,
      const criterion::CostVector &tension,
      const bool enableEarlyRecovery,
      SubnetSpace &space /* to be filled */) const;

  const criterion::Criterion &criterion;
  const CutProvider cutProvider;
  const MatchFinder matchFinder;
  const CellEstimator cellEstimator;
  const CostAggregator costAggregator;
  const CostPropagator costPropagator;
};

} // namespace eda::gate::techmapper
