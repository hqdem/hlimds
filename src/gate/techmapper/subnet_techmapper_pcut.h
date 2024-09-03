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
                       const MatchFinder matchFinder,
                       const CellEstimator cellEstimator,
                       const CostAggregator costAggregator,
                       const CostPropagator costPropagator);

  SubnetTechMapperPCut(const std::string &name,
                       const criterion::Criterion &criterion,
                       const uint16_t maxCutSize,
                       const MatchFinder matchFinder,
                       const CellEstimator cellEstimator);

protected:
  void onBegin(const SubnetBuilderPtr &oldBuilder) override {
    cutExtractor = std::make_unique<optimizer::CutExtractor>(
        oldBuilder.get(), maxCutSize, true /* extract now */);
  }

  void onRecovery(const Status &status,
                  criterion::CostVector &tension) override;

private:
  const uint16_t maxCutSize;
  std::unique_ptr<optimizer::CutExtractor> cutExtractor;
};

} // namespace eda::gate::techmapper
