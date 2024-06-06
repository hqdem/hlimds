//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/celltype.h"
#include "gate/model/subnet.h"
#include "gate/techmapper/library/liberty_manager.h"

#include "kitty/kitty.hpp"

#include <readcells/groups.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace eda::gate::techmapper {

using CellTypeID = eda::gate::model::CellTypeID;
using CellSymbol = eda::gate::model::CellSymbol;

class SC {
public:
  SC();
  using StandardSeqMap = std::unordered_map<CellSymbol, std::vector<CellTypeID>>;

  std::vector<CellTypeID> getCombCellTypeID() const {
    return combCellTypeIDs;
  }

  StandardSeqMap getSeqCellTypeID() const {
    return seqCellTypeIDs;
  }

private:
  std::vector<CellTypeID> combCellTypeIDs;
  StandardSeqMap seqCellTypeIDs;

  void readLibertyFile();
  void processCell(const Cell &cell);
  bool isCombCell(const Cell &cell, const std::vector<std::string> &inputs,
                  const std::vector<std::string> &outputs,
                  const std::vector<std::string> &funcs) const;
  void createCombCellType(const std::string &name,
                          const std::vector<std::string> &inputs,
                          const std::string &func,
                          const model::CellTypeAttrID cellTypeAttrID);
  /*void createSeqCellType(const Cell &cell,
                         const std::vector<std::string> &inputs,
                         const std::string &func,
                         model::CellTypeAttrID cellTypeAttrID);*/
};

} // namespace eda::gate::techmapper