//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#pragma once

#include "gate/model2/subnet.h"
#include "gate/optimizer2/cut_extractor.h"
#include "gate/techoptimizer/cut_based_tech_mapper/strategy/bestReplacement.h"
#include "gate/techoptimizer/library/cellDB.h"

#include "gate/techoptimizer/cut_based_tech_mapper/strategy/strategy.h"

/**
 * \brief Simplified strategy for finding best replacements.
 * \author <a href="mailto:dGaryaev@ispras.ru">Daniil Gariaev</a>
 */

namespace eda::gate::tech_optimizer {

class SimplifiedStrategy : public Strategy{

  void findBest(EntryIndex entryIndex, const CutsList &CutsList,
                std::map<EntryIndex, BestReplacement> &bestReplacementMap,
                CellDB &cellDB,
                SubnetID subnetID) override;

};

} // namespace eda::gate::tech_optimizer

