//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/library/library.h"
#include "gate/model/cell.h"
#include "gate/techmapper/techmapper.h"

#include <vector>

namespace eda::gate::techmapper {

class SequentialMapper {
  using SCLibrary = library::SCLibrary;
  using CellID = model::CellID;
  using CellTypeID = model::CellTypeID;
  using SCAttrs = library::SCAttrs;
  using Strategy = Techmapper::Strategy;
  using SubnetID = model::SubnetID;

public:
  explicit SequentialMapper(const SCLibrary *cellDB) {
    this->cellDB = cellDB;
  };
  CellTypeID map(const CellID sequenceCellID,
    const Strategy strategy = Strategy::AREA);

private:
  const SCLibrary *cellDB;
  SubnetID findSubnetID(
    std::vector<std::pair<SubnetID, SCAttrs>> &seqCells,
    const Strategy strategy = Strategy::AREA);
};

} // namespace eda::gate::techmapper
