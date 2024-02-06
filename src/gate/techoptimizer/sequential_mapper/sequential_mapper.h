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

#include <memory>
#include <utility>
#include <vector>

namespace eda::gate::tech_optimizer {

class SequentialMapper {
public:
  explicit SequentialMapper(CellDB *cellDB);
  model::SubnetID mapSequenceCell(model::CellID sequenceCellID, Techmapper::MapperType techmapSelector);

private:
  CellDB *cells;
  model::SubnetID mapLatch(Techmapper::MapperType techmapSelector);
  model::SubnetID mapDFF(Techmapper::MapperType techmapSelector);
  model::SubnetID mapDFFrs(Techmapper::MapperType techmapSelector);
  model::SubnetID chooseMappingStrategy(const std::vector<std::pair<model::SubnetID, Subnetattr>>& seqCells,
                                        Techmapper::MapperType techmapSelector);
  model::SubnetID areaOptimizedMapping(const std::vector<std::pair<model::SubnetID, Subnetattr>>& seqCells);
};

} // namespace eda::gate::tech_optimizer