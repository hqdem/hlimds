//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "context/utopia_context.h"
#include "gate/criterion/solution_space.h"
#include "gate/model/subnet.h"
#include "gate/optimizer/cut.h"
#include "gate/optimizer/transformer.h"

#include <cmath>
#include <cstddef>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace eda::gate::techmapper {

/**
 * @brief General dynamic programming-based subnet mapper.
 */
class SubnetTechMapperBase : public optimizer::SubnetTransformer {
public:
  static constexpr auto enableConstMapping = false;

  struct Match final {
    model::CellTypeID typeID{model::OBJ_NULL_ID};
    model::Subnet::LinkList links{};
    uint16_t output{0};
    bool inversion{false};
  };

  struct CellContext final {
    /// Logic-level fanout.
    uint16_t fanout{0};
  };

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
                        const CellContext &,
                        const context::TechMapContext &)>;
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
                       const context::UtopiaContext &context,
                       const CutProvider cutProvider,
                       const MatchFinder matchFinder,
                       const CellEstimator cellEstimator,
                       const CostAggregator costAggregator,
                       const CostPropagator costPropagator):
      optimizer::SubnetTransformer(name),
      context(context),
      cutProvider(cutProvider),
      matchFinder(matchFinder),
      cellEstimator(cellEstimator),
      costAggregator(costAggregator),
      costPropagator(costPropagator) {}

  SubnetTechMapperBase(const std::string &name,
                       const context::UtopiaContext &context,
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
           const criterion::CostVector &vector,
           const criterion::CostVector &tension):
        verdict(verdict),
        isFeasible(isFeasible),
        vector(vector),
        tension(tension) {}

    Status(const Verdict verdict,
           const criterion::CostVector &vector,
           const criterion::CostVector &tension):
        Status(verdict, false, vector, tension) {}

    Verdict verdict{UNSAT};
    bool isFeasible{false};
    criterion::CostVector vector;
    criterion::CostVector tension;
  };

  virtual void onBegin(const SubnetBuilderPtr &oldBuilder);

  virtual bool onRecovery(const SubnetBuilderPtr &oldBuilder,
                          const Status &status);

  virtual void onEnd(const SubnetBuilderPtr &newBuilder) {}

  bool hasSolutions(const optimizer::Cut &cut) const {
    for (const auto leafID : cut.leafIDs) {
      if (!space[leafID]->hasSolution()) {
        return false;
      }
    }
    return true;
  }

  std::vector<criterion::CostVector> getCostVectors(
      const optimizer::Cut &cut) const {
    std::vector<criterion::CostVector> vectors;
    vectors.reserve(cut.leafIDs.size());
    for (const auto leafID : cut.leafIDs) {
      vectors.push_back(space[leafID]->getBest().vector);
    }
    return vectors;
  }

  Status techMap(const SubnetBuilderPtr &builder);

  void findCellSolutions(const SubnetBuilderPtr &builder,
                         const model::EntryID entryID,
                         const optimizer::CutsList &cuts);

  std::vector<Match> &getMatches(const model::SubnetBuilder &builder,
                                 const optimizer::Cut &cut) {
    const auto i = cutMatches.find(cut);
    if (i != cutMatches.end()) {
      return i->second;
    }

    const auto j = cutMatches.emplace(cut, matchFinder(builder, cut));
    return j.first->second;
  }

  // Maximum number of tries for recovery.
  const uint16_t maxTries{3};

  const context::UtopiaContext &context;
  const CutProvider cutProvider;
  const MatchFinder matchFinder;
  const CellEstimator cellEstimator;
  const CostAggregator costAggregator;
  const CostPropagator costPropagator;

  // State modified during technology mapping.
  uint16_t tryCount;
  SubnetSpace space;

  criterion::CostVector vector;
  criterion::CostVector tension;

  // Cache of matches (to speed up multiple tries).
  std::unordered_map<optimizer::Cut, std::vector<Match>> cutMatches;
};

} // namespace eda::gate::techmapper
