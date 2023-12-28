//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#pragma once

#include "gate/model2/net.h"

using NetID = eda::gate::model::NetID;
using SubnetID = eda::gate::model::SubnetID;

namespace eda::gate::tech_optimizer {

class Techmaper {
public:
  enum class TechmaperType {
    FUNC,
    STRUCT
  };

  enum class TechmaperStrategyType {
    AREA_FLOW,
    DELAY,
    POWER,
    SIMPLE
  };

  void setLiberty(const std::string &dbPath);
  void setMapper(TechmaperType techmapSelector);
  void setStrategy(TechmaperStrategyType strategySelector);
  SubnetID techmap(SubnetID subnetID);
  SubnetID techmap(model::CellID sequenceCell);
};
} // namespace eda::gate::tech_optimizer
