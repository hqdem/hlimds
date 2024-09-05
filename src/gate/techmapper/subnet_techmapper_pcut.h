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
                       const criterion::Criterion &criterion,
                       const uint16_t maxCutSize,
                       const uint16_t maxCutNum,
                       const MatchFinder matchFinder,
                       const CellEstimator cellEstimator,
                       const CostAggregator costAggregator,
                       const CostPropagator costPropagator);

  SubnetTechMapperPCut(const std::string &name,
                       const criterion::Criterion &criterion,
                       const uint16_t maxCutSize,
                       const uint16_t maxCutNum,
                       const MatchFinder matchFinder,
                       const CellEstimator cellEstimator);

protected:
  void onBegin(const SubnetBuilderPtr &oldBuilder) override {
    SubnetTechMapperBase::onBegin(oldBuilder);

    cutExtractor = std::make_unique<optimizer::CutExtractor>(
        oldBuilder.get(), maxCutSize, false /* extract on demand */);
  }

private:
  void computePriorityCuts(const model::EntryID entryID);

  const uint16_t maxCutSize;
  const uint16_t maxCutNum;

  std::unique_ptr<optimizer::CutExtractor> cutExtractor;
};

} // namespace eda::gate::techmapper
