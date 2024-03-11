//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#pragma once

#include "gate/model2/net.h"
#include "gate/techoptimizer/library/cell.h"
#include "gate/techoptimizer/library/cellDB.h"
#include "gate/techoptimizer/mapper/baseMapper.h"

using NetID = eda::gate::model::NetID;
using SubnetID = eda::gate::model::SubnetID;

namespace eda::gate::tech_optimizer {

class Techmapper {
public:
  enum class MapperType {
    AREA_FLOW,
    DELAY,
    POWER,
    SIMPLE_AREA_FUNC
  };

  Techmapper(const std::string &dbPath, MapperType techmapSelector, SDC& sdc);

  SubnetID techmap(SubnetID subnetID);
  NetID techmap(NetID netID);
  SubnetID techmap(model::CellID sequenceCell, MapperType techmapSelector = MapperType::SIMPLE_AREA_FUNC);

  ~Techmapper() {
    delete mapper;
    delete cellDB;
  }

private:
  void setLiberty(const std::string &dbPath);
  void setMapper(MapperType techmapSelector);
  SDC sdc;
  CellDB *cellDB;
  BaseMapper *mapper = nullptr;
  SubnetID premapAIGSubnet(SubnetID subnetID);
};
} // namespace eda::gate::tech_optimizer
