//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#pragma once

#include "gate/model2/net.h"

namespace eda::gate::tech_optimizer {

using NetID = eda::gate::model::NetID;
using SubnetID = eda::gate::model::SubnetID;

enum class TechmapType {
  FUNC,
  STRUCT
};

enum class StrategyType {
  AREA_FLOW,
  DELAY,
  POWER
};

void setLiberty(const std::string &dbPath);
void setMapper(TechmapType techmapSelector);
void setStrategy(StrategyType strategySelector);
SubnetID techmap(SubnetID subnetID);
SubnetID techmap(model::Subnet::Cell sequenceCell);
} // namespace eda::gate::tech_optimizer
