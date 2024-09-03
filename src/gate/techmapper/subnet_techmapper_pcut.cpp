//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "subnet_techmapper_pcut.h"

#include <cassert>

namespace eda::gate::techmapper {

#define UTOPIA_CUT_PROVIDER_LAMBDA() \
  [this](const model::SubnetBuilder &builder,\
         const model::EntryID entryID) -> optimizer::CutsList {\
    return cutExtractor->getCuts(entryID);\
  }

SubnetTechMapperPCut::SubnetTechMapperPCut(
    const std::string &name,
    const criterion::Criterion &criterion,
    const uint16_t maxCutSize,
    const MatchFinder matchFinder,
    const CellEstimator cellEstimator,
    const CostAggregator costAggregator,
    const CostPropagator costPropagator):
    SubnetTechMapperBase(name,
                         criterion,
                         UTOPIA_CUT_PROVIDER_LAMBDA(),
                         matchFinder,
                         cellEstimator,
                         costAggregator,
                         costPropagator),
    maxCutSize(maxCutSize) {}

SubnetTechMapperPCut::SubnetTechMapperPCut(
    const std::string &name,
    const criterion::Criterion &criterion,
    const uint16_t maxCutSize,
    const MatchFinder matchFinder,
    const CellEstimator cellEstimator):
    SubnetTechMapperBase(name,
                         criterion,
                         UTOPIA_CUT_PROVIDER_LAMBDA(),
                         matchFinder,
                         cellEstimator),
    maxCutSize(maxCutSize) {}

void SubnetTechMapperPCut::onRecovery(
    const Status &status, criterion::CostVector &tension) {
  tension *= status.tension;
}

} // namespace eda::gate::techmapper
