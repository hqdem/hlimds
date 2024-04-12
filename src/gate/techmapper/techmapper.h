//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/net.h"
#include "gate/techmapper/library/cell.h"
#include "gate/techmapper/library/cell_db.h"
#include "gate/techmapper/comb_mapper/comb_mapper.h"

using NetID = eda::gate::model::NetID;
using SubnetID = eda::gate::model::SubnetID;

namespace eda::gate::techmapper {

class Techmapper {
public:
  enum class MapperType {
    AREA_FLOW,
    DELAY,
    POWER,
    SIMPLE_AREA_FUNC,
    GENETIC,
    SIMPLE_DELAY_FUNC,
  };

  Techmapper(const std::string &dbPath, MapperType techmapSelector, SDC& sdc);

  SubnetID techmap(SubnetID subnetID);
  NetID techmap(NetID netID);
  model::CellTypeID techmap(model::CellID sequenceCell, MapperType techmapSelector = MapperType::SIMPLE_AREA_FUNC);

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
  NetID sequenseTechMapping(NetID netID);
};
} // namespace eda::gate::techmapper
