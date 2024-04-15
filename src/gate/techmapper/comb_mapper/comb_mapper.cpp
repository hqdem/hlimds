//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/techmapper/comb_mapper/comb_mapper.h"

namespace eda::gate::techmapper {

void BaseMapper::mapping(SubnetID subnetID,
                     CellDB *cellDB,
                     SDC &sdc,
    std::unordered_map<EntryIndex, BestReplacement> *bestReplacementMap) {
  this->subnetID = subnetID;
  this->cellDB = cellDB;
  this->sdc = sdc;
  this->bestReplacementMap = bestReplacementMap;

  baseMap();
}

} // namespace eda::gate::techmapper
