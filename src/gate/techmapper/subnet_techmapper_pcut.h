//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "subnet_techmapper_base.h"

namespace eda::gate::techmapper {

/**
 * @brief Subnet mapper based on priority cuts.
 */
class SubnetTechMapperPCut final : public SubnetTechMapperBase {
public:
  SubnetTechMapperPCut(const std::string &name,
                       const criterion::Criterion &criterion,
                       const CutProvider cutProvider, // TODO: max cut size
                       const MatchFinder matchFinder,
                       const CellEstimator cellEstimator,
                       const CostAggregator costAggregator,
                       const CostPropagator costPropagator);

  SubnetTechMapperPCut(const std::string &name,
                       const criterion::Criterion &criterion,
                       const CutProvider cutProvider, // TODO: max cut size
                       const MatchFinder matchFinder,
                       const CellEstimator cellEstimator);

protected:
  void onRecovery(const Status &status,
                  criterion::CostVector &tension) const override;

private:
  // Cut extractor.
};

} // namespace eda::gate::techmapper
