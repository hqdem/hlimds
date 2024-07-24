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
#include "util/singleton.h"

#include <kitty/print.hpp>

#include <readcells/ast.h>
#include <readcells/ast_parser.h>
#include <readcells/groups.h>
#include <readcells/token_parser.h>

#include <cstdio>
#include <filesystem>
#include <memory.h>
#include <regex>
#include <string>

namespace eda::gate::library {

class SCLibrary : public util::Singleton<SCLibrary> {
  using CellTypeID = model::CellTypeID;

public:
  struct StandardCell {
    CellTypeID cellTypeID;
    std::vector<int> link;
  };

  void loadCells();
  void addCell(CellTypeID cell);
  std::vector<StandardCell> getCombCells();

private:
  std::vector<StandardCell> combCells;

  void permutation();
  void addCombCell(std::string name);

  SCLibrary() : Singleton() {}
  friend class Singleton<SCLibrary>;
};

} // namespace eda::gate::library
