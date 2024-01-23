//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#pragma once

#include "gate/model2/net.h"
#include "gate/techoptimizer/baseMapper.h"
#include "gate/techoptimizer/library/cell.h"
#include "gate/techoptimizer/library/cellDB.h"

using NetID = eda::gate::model::NetID;
using SubnetID = eda::gate::model::SubnetID;

namespace eda::gate::tech_optimizer {

class Techmapper {
public:
  enum class TechmapperType {
    FUNC,
    STRUCT
  };

  enum class TechmapperStrategyType {
    AREA_FLOW,
    DELAY,
    POWER,
    SIMPLE
  };

  void setLiberty(const std::string &dbPath);
  void setMapper(TechmapperType techmapSelector);
  void setStrategy(TechmapperStrategyType strategySelector);
  SubnetID techmap(SubnetID subnetID);
  SubnetID techmap(model::CellID sequenceCell);

  std::vector<Cell*> cells;
  CellDB *cellDB;
  BaseMapper *mapper = nullptr;

  ~Techmapper() {
    delete mapper;
    delete cellDB;
    for(auto *cell : cells) {
      delete cell;
    }
  }
};
} // namespace eda::gate::tech_optimizer
