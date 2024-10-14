//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/cut_extractor.h"
#include "gate/techmapper/subnet_techmapper_base.h"

#include <memory>

namespace eda::gate::techmapper {

/**
 * @brief Subnet mapper based on priority cuts.
 */
class SubnetTechMapperPCut final : public SubnetTechMapperBase {
public:
  SubnetTechMapperPCut(const std::string &name,
                       const context::UtopiaContext &context,
                       const uint16_t maxCutSize,
                       const uint16_t maxCutNum,
                       const CutEstimator cutEstimator,
                       const MatchFinder matchFinder,
                       const CellEstimator cellEstimator,
                       const CostAggregator costAggregator,
                       const CostPropagator costPropagator);

  SubnetTechMapperPCut(const std::string &name,
                       const context::UtopiaContext &context,
                       const uint16_t maxCutSize,
                       const uint16_t maxCutNum,
                       const CutEstimator cutEstimator,
                       const MatchFinder matchFinder,
                       const CellEstimator cellEstimator);

  SubnetTechMapperPCut(const std::string &name,
                       const context::UtopiaContext &context,
                       const uint16_t maxCutSize,
                       const uint16_t maxCutNum,
                       const MatchFinder matchFinder,
                       const CellEstimator cellEstimator);

protected:
  void onBegin(const SubnetBuilderPtr &oldBuilder) override {
    SubnetTechMapperBase::onBegin(oldBuilder);

    cutsPerCell = maxCutNum;
    cutExtractor = std::make_unique<optimizer::CutExtractor>(
        oldBuilder.get(), maxCutSize, false /* extract on demand */);
  }

  bool onRecovery(const SubnetBuilderPtr &oldBuilder,
                  const Status &status) override {
    if (!SubnetTechMapperBase::onRecovery(oldBuilder, status)) {
      cutsPerCell = static_cast<uint16_t>(1.5 * cutsPerCell);
    } else {
      cutsPerCell += 2;
    }
    return true;
  }

private:
  criterion::Cost estimateCut(const SubnetBuilderPtr &builder,
                              const optimizer::Cut &cut,
                              const CellContext &cellContext) {
    const auto prevVector = costAggregator(getCostVectors(cut));
    const auto cellVector = cutEstimator(*builder, cut, cellContext);
    const auto costVector = prevVector + cellVector;
    return context.criterion->getPenalizedCost(costVector, tension);
  }

  void computePCuts(const SubnetBuilderPtr &builder,
                    const model::EntryID entryID);

  const uint16_t maxCutSize;
  const uint16_t maxCutNum;

  uint16_t cutsPerCell;
  std::unique_ptr<optimizer::CutExtractor> cutExtractor;
};

} // namespace eda::gate::techmapper
