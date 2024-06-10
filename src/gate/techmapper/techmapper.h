//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/library/library.h"
#include "gate/library/sdc_manager.h"
#include "gate/model/net.h"
#include "gate/optimizer/subnet_transformer.h"
#include "gate/techmapper/comb_mapper/comb_mapper.h"

#include <filesystem>

namespace eda::gate::techmapper {

class Techmapper {
  using CellID = model::CellID;
  using CellTypeID = model::CellTypeID;
  using NetID = model::NetID;
  using SCLibrary = library::SCLibrary;
  using SDC = library::SDC;
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

  void setSDC(const std::filesystem::path &sdcPath);
  void setStrategy(const Strategy strategy);
  void setLibrary(const std::filesystem::path &libPath);

private:
  void createCellDB();
  const SDC *sdc = nullptr;
  const SCLibrary *cellDB = nullptr;
  CombMapper *mapper = nullptr;
  NetID seqTechmap(const NetID netID);
};
} // namespace eda::gate::techmapper
