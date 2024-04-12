//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/techmapper/comb_mapper/best_replacement.h"
#include "gate/techmapper/library/cell_db.h"
#include "gate/techmapper/library/sdc.h"

#include <map>

namespace eda::gate::techmapper {

class BaseMapper {
public:
  void mapping(SubnetID subnetID,
           CellDB *cellDB,
           SDC &sdc,
           std::unordered_map<EntryIndex, BestReplacement> *bestReplacementMap);

  virtual ~BaseMapper() = default;
protected:
  CellDB *cellDB;
  SubnetID subnetID;
  SDC sdc;
  std::unordered_map<EntryIndex, BestReplacement> *bestReplacementMap;

  virtual void baseMap() = 0;
};

} // namespace eda::gate::techmapper
