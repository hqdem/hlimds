//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/net.h"
#include "gate/techmapper/comb_mapper/comb_mapper.h"
#include "gate/techmapper/library/cell_db.h"

namespace eda::gate::techmapper {

class Techmapper {

  using CellID = model::CellID;
  using CellTypeID = model::CellTypeID;
  using NetID = model::NetID;
  using SubnetID = model::SubnetID;

public:
  enum Strategy {
    AREA,
    AREA_FLOW,
    DELAY,
    GENETIC,
    POWER
  };

  Techmapper(const Strategy strategy, const SDC &sdc, const std::string &libPath = "");

  virtual ~Techmapper() {
    delete mapper;
    delete cellDB;
  }

  SubnetID techmap(const SubnetID subnetID);
  NetID techmap(const NetID netID);
  CellTypeID techmap(const CellID sequenceCell,
                     const Strategy strategy = Strategy::AREA);

private:
  void createCellDB();
  void setMapper(const Strategy strategy);
  SDC sdc;
  CellDB *cellDB;
  BaseMapper *mapper = nullptr;
  SubnetID premapAIGSubnet(const SubnetID subnetID);
  NetID sequenseTechMapping(const NetID netID);
};
} // namespace eda::gate::techmapper
