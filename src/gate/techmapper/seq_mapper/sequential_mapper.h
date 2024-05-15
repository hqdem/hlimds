//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/cell.h"
#include "gate/techmapper/library/cell_db.h"
#include "gate/techmapper/techmapper.h"

#include <memory>
#include <utility>
#include <vector>

namespace eda::gate::techmapper {

class SequentialMapper {
public:
  explicit SequentialMapper(CellDB *cellDB);
  model::CellTypeID mapSequenceCell(model::CellID sequenceCellID, Techmapper::MapperType techmapSelector);

private:
  CellDB *cells;
  model::SubnetID mapLatch(Techmapper::MapperType techmapSelector);
  model::SubnetID mapDFF(Techmapper::MapperType techmapSelector);
  model::SubnetID mapDFFrs(Techmapper::MapperType techmapSelector);
  model::SubnetID chooseMappingStrategy(const std::vector<std::pair<model::SubnetID, Subnetattr>>& seqCells,
                                        Techmapper::MapperType techmapSelector);
  model::SubnetID areaOptimizedMapping(const std::vector<std::pair<model::SubnetID, Subnetattr>>& seqCells);
};

} // namespace eda::gate::techmapper
