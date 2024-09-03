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
class SubnetTechMapperBase : public optimizer::SubnetTransformer {
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
                        const model::EntryID entryID)>;
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

  using SubnetBuilderPtr = std::shared_ptr<model::SubnetBuilder>;

  SubnetTechMapperBase(const std::string &name,
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

  SubnetTechMapperBase(const std::string &name,
                       const criterion::Criterion &criterion,
                       const CutProvider cutProvider,
                       const MatchFinder matchFinder,
                       const CellEstimator cellEstimator);

  SubnetBuilderPtr map(const SubnetBuilderPtr &builder) const override;

  virtual ~SubnetTechMapperBase() {}

protected:
  struct Status final {
    enum Verdict {
      /// Solution is found.
      FOUND,
      /// Solution does not exist.
      UNSAT,
      /// Early recovery.
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

  virtual void onBegin(const SubnetBuilderPtr &oldBuilder) {}

  virtual void onRecovery(const Status &status,
                          criterion::CostVector &tension) {
    tension *= status.tension;
  }

  virtual void onEnd(const SubnetBuilderPtr &newBuilder) {}

  Status map(const SubnetBuilderPtr &builder,
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
