//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/celltype.h"
#include "util/singleton.h"

namespace eda::gate::library {

class SCLibrary : public util::Singleton<SCLibrary> {
  using CellTypeID = model::CellTypeID;

public:
  struct StandardCell {
    CellTypeID cellTypeID;
    std::vector<int> link;
  };

  void loadCells();
  void addCell(CellTypeID typeID);
  std::vector<StandardCell> getCombCells();

private:
  std::vector<StandardCell> combCells;

  void permutation();
  void addCombCell(const std::string &name);

  SCLibrary() : Singleton() {}
  friend class Singleton<SCLibrary>;
};

} // namespace eda::gate::library
