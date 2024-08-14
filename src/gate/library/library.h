//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/library/readcells_iface.h"
#include "gate/model/celltype.h"

#include <readcells/groups.h>
#include <readcells/token_parser.h>

#include <string>

namespace eda::gate::library {

class SCLibrary {
  using CellTypeID = model::CellTypeID;

public:
  SCLibrary(const std::string &fileName);
  virtual ~SCLibrary();

  struct StandardCell {
    CellTypeID cellTypeID;
    std::vector<int> link;
  };

  std::vector<StandardCell> getCombCells();

  Library &getLibrary() {
    return library;
  }

private:
  void loadCells();
  void addCell(CellTypeID typeID);

  // ReadCells
  Group *ast;
  Library library;
  TokenParser tokParser;
  ReadCellsIface *iface;

  std::vector<StandardCell> combCells;

  void permutation();
  void addCombCell(const std::string &name);
};

extern SCLibrary *library;

} // namespace eda::gate::library
