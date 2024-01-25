//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#pragma once

#include "gate/model2/net.h"
#include "gate/techoptimizer/mapper/baseMapper.h"
#include "gate/techoptimizer/library/cell.h"
#include "gate/techoptimizer/library/cellDB.h"

using NetID = eda::gate::model::NetID;
using SubnetID = eda::gate::model::SubnetID;

namespace eda::gate::tech_optimizer {

class Techmapper {
public:
  enum class MapperType {
    AREA_FLOW,
    DELAY,
    POWER,
    SIMPLE
  };

  void setLiberty(const std::string &dbPath);
  void setMapper(MapperType techmapSelector);
  SubnetID techmap(SubnetID subnetID);
  SubnetID techmap(model::CellID sequenceCell);

  ~Techmapper() {
    delete mapper;
    delete cellDB;
    for(auto *cell : cells) {
      delete cell;
    }
  }

private:
  std::vector<Cell*> cells;
  CellDB *cellDB;
  BaseMapper *mapper = nullptr;
  SubnetID premapAIGSubnet(SubnetID subnetID);
};
} // namespace eda::gate::tech_optimizer
