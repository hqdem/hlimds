//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/techoptimizer/mapper/baseMapper.h"

namespace eda::gate::tech_optimizer {
void BaseMapper::mapping(SubnetID subnetID,
                     CellDB *cellDB,
    std::unordered_map<EntryIndex, BestReplacement> *bestReplacementMap) {
  this->subnetID = subnetID;
  this->cellDB = cellDB;
  this->bestReplacementMap = bestReplacementMap;

  baseMap();
}
} // namespace eda::gate::tech_optimizer