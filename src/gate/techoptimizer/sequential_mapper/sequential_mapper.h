//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/cell.h"
#include "gate/techoptimizer/library/cellDB.h"
#include "gate/techoptimizer/techoptimizer.h"

#include <list>

namespace eda::gate::tech_optimizer {

  void setSequenceDB(CellDB *cellDB);
  model::SubnetID mapSequenceCell(model::CellID sequenceCell,
                                  Techmapper::MapperType techmapSelector);
} // namespace eda::gate::tech_optimizer
