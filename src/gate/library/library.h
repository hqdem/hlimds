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
#include "util/kitty_utils.h"

#include <readcells/groups.h>
#include <readcells/token_parser.h>

#include <string>

namespace eda::gate::library {

class SCLibrary {
public:
  SCLibrary(const std::string &fileName);
  virtual ~SCLibrary();

  struct StandardCell {
    model::CellTypeID cellTypeID;
    kitty::dynamic_truth_table ctt; // canonized TT
    utils::NpnTransformation transform;
  };

  std::vector<StandardCell> &getCombCells() {
    return combCells;
  }

  Library &getLibrary() {
    return library;
  }

private:
  std::vector<StandardCell> combCells;

  void loadCombCell(const std::string &name);

  // ReadCells
  Group *ast;
  Library library;
  TokenParser tokParser;
  ReadCellsIface *iface;
};

extern SCLibrary *library;

} // namespace eda::gate::library
