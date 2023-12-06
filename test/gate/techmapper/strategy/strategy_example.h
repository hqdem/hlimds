//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#pragma once

#include "gate/model2/subnet.h"
#include "gate/techoptimizer/library/cellDB.h"

using SubnetID = eda::gate::model::SubnetID;

namespace eda::gate::tech_optimizer {

  CellDB getSimpleCells();
  SubnetID subnet1();
  SubnetID subnet2();

} // namespace eda::gate::optimizer
