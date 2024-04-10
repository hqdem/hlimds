//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/celltype.h"
#include "gate/model2/subnet.h"

#include "kitty/kitty.hpp"

#include <string>
#include <vector>

namespace eda::gate::tech_optimizer {

using CellTypeID = eda::gate::model::CellTypeID;

struct LibraryCells {
  LibraryCells() = default;

  static void readLibertyFile(const std::string &filename, std::vector<CellTypeID> &cellTypeIDs,
                                     std::vector<CellTypeID> &cellTypeFFIDs,
                                     std::vector<CellTypeID> &cellTypeFFrsIDs,
                                     std::vector<CellTypeID> &cellTypeLatchIDs);
};

} // namespace eda::gate::tech_optimizer
