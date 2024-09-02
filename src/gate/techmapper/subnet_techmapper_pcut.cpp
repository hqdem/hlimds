//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "subnet_techmapper_pcut.h"

namespace eda::gate::techmapper {

SubnetTechMapperPCut::SubnetTechMapperPCut(
    const std::string &name,
    const criterion::Criterion &criterion,
    const CutProvider cutProvider, // TODO: max cut size
    const MatchFinder matchFinder,
    const CellEstimator cellEstimator,
    const CostAggregator costAggregator,
    const CostPropagator costPropagator):
    SubnetTechMapperBase(name,
                         criterion,
                         cutProvider,
                         matchFinder,
                         cellEstimator,
                         costAggregator,
                         costPropagator) {}

SubnetTechMapperPCut::SubnetTechMapperPCut(
    const std::string &name,
    const criterion::Criterion &criterion,
    const CutProvider cutProvider, // TODO: max cut size
    const MatchFinder matchFinder,
    const CellEstimator cellEstimator):
    SubnetTechMapperBase(name,
                         criterion,
                         cutProvider,
                         matchFinder,
                         cellEstimator) {}

void SubnetTechMapperPCut::onRecovery(
    const Status &status, criterion::CostVector &tension) const {
  // TODO:
  tension *= status.tension;
}

} // namespace eda::gate::techmapper
