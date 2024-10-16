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
  static constexpr auto enableConstMapping = true;

  struct Match final {
    model::CellTypeID typeID{model::OBJ_NULL_ID};
    model::Subnet::LinkList links{};
    uint16_t output{0};
    bool inversion{false};
  };

  struct CellContext final {
    struct LinkInfo final {
      model::CellTypeID typeID{model::OBJ_NULL_ID};
      uint16_t output{0};
    };
    /// Information on the link types.
    std::vector<LinkInfo> links{};
    /// Logic-level fanout.
    size_t fanout{0};
  };

  using CutProvider =
      std::function<optimizer::CutsList(
                        const std::shared_ptr<model::SubnetBuilder> &,
                        const model::EntryID entryID)>;
  using CutEstimator =
      std::function<criterion::CostVector(
                        const model::SubnetBuilder &,
                        const optimizer::Cut &,
                        const CellContext &)>;
  using MatchFinder =
      std::function<std::vector<Match>(
                        const std::shared_ptr<model::SubnetBuilder> &,
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
                       const CutEstimator cutEstimator,
                       const MatchFinder matchFinder,
                       const CellEstimator cellEstimator,
                       const CostAggregator costAggregator,
                       const CostPropagator costPropagator);

  SubnetTechMapperBase(const std::string &name,
                       const context::UtopiaContext &context,
                       const CutProvider cutProvider,
                       const CutEstimator cutEstimator,
                       const MatchFinder matchFinder,
                       const CellEstimator cellEstimator);

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

  CellContext::LinkInfo getLinkInfo(const model::SubnetBuilder &builder,
                                    const model::EntryID sourceID,
                                    uint16_t sourcePort) const;

  CellContext getCellContext(const model::SubnetBuilder &builder,
                             const optimizer::Cut &cut) const;

  CellContext getCellContext(const model::SubnetBuilder &builder,
                             const model::EntryID entryID,
                             const Match &match) const;

  criterion::CostVector estimateCutVector(const SubnetBuilderPtr &builder,
                                          const optimizer::Cut &cut) const {
    const auto cellContext = getCellContext(*builder, cut);
    const auto prevVector = costAggregator(getCostVectors(cut));
    const auto cellVector = cutEstimator(*builder, cut, cellContext);
    return prevVector + cellVector;
  }

  criterion::Cost estimateCutCost(const SubnetBuilderPtr &builder,
                                  const optimizer::Cut &cut,
                                  const bool penalize = false) const {
    const auto costVector = estimateCutVector(builder, cut);

    return penalize
        ? context.criterion->getPenalizedCost(costVector, tension)
        : context.criterion->getCost(costVector);
  }

  std::vector<Match> &getMatches(const SubnetBuilderPtr &builder,
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
  const CutEstimator cutEstimator;
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

criterion::CostVector defaultCutEstimator(
    const model::SubnetBuilder &builder,
    const optimizer::Cut &cut,
    const SubnetTechMapperBase::CellContext &cellContext);

criterion::CostVector defaultCostAggregator(
    const std::vector<criterion::CostVector> &vectors);

criterion::CostVector defaultCostPropagator(
    const criterion::CostVector &vector,
    const uint32_t fanout);

} // namespace eda::gate::techmapper
