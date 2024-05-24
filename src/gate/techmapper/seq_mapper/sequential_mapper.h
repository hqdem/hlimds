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

#include <vector>

namespace eda::gate::techmapper {

class SequentialMapper {
  using CellID = model::CellID;
  using CellTypeID = model::CellTypeID;
  using Strategy = Techmapper::Strategy;
  using SubnetID = model::SubnetID;

public:
  explicit SequentialMapper(CellDB *cellDB) {
    this->cellDB = cellDB;
  };
  CellTypeID map(const CellID sequenceCellID,
    const Strategy strategy = Strategy::AREA);

private:
  CellDB *cellDB;
  SubnetID findSubnetID(
    const std::vector<std::pair<SubnetID, Subnetattr>> &seqCells,
    const Strategy strategy = Strategy::AREA);
};

} // namespace eda::gate::techmapper
