//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/net.h"
#include "gate/optimizer/subnet_transformer.h"
#include "gate/techmapper/comb_mapper/comb_mapper.h"
#include "gate/techmapper/library/cell_db.h"
#include "gate/techmapper/library/sdc.h"

#include <filesystem>

namespace eda::gate::techmapper {

class Techmapper {
  using CellID = model::CellID;
  using CellTypeID = model::CellTypeID;
  using NetID = model::NetID;
  using SubnetBuilder = model::SubnetBuilder;
  using SubnetID = model::SubnetID;

public:
  enum Strategy {
    AREA,
    AREA_FLOW,
    DELAY,
    GENETIC,
    POWER
  };

  optimizer::SubnetBuilderPtr make(const SubnetID subnetID) const;

  virtual ~Techmapper() {
    if (mapper != nullptr) {
      delete mapper;
    }
    if (cellDB != nullptr) {
      delete cellDB;
    }
  }

  void techmap(const SubnetID subnetID,
               SubnetBuilder &builder) const;
  NetID techmap(const NetID netID);
  CellTypeID techmap(const CellID sequenceCell,
                     const Strategy strategy = Strategy::AREA);

  void setSDC(const SDC &sdc) { this->sdc = &sdc; }
  void setStrategy(const Strategy strategy);
  void setLibrary(const std::filesystem::path &libPath);

private:
  void createCellDB();
  const SDC *sdc = nullptr;
  const CellDB *cellDB = nullptr;
  CombMapper *mapper = nullptr;
  NetID seqTechmap(const NetID netID);
};
} // namespace eda::gate::techmapper
