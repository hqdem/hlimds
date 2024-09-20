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
    std::vector<kitty::dynamic_truth_table> ctt; // canonized TT
    std::vector<util::NpnTransformation> transform;
  };

  std::vector<StandardCell> &getCombCells() {
    return combCells;
  }

  const Library &getLibrary() const  {
    return library;
  }

  //TODO: should be removed. Used only in readcells_iface_test
  Library &getLibraryRaw() {
    return library;
  }

  inline uint getMaxArity() const { return maxArity; }

private:
  uint maxArity = 0;
  std::vector<StandardCell> combCells;
  std::vector<StandardCell> negCombCells;
  std::vector<StandardCell> constOneCells;
  std::vector<StandardCell> constZeroCells;

  // const StandardCell *cheapBuf; TODO: add buffer to avoid using consts
  const StandardCell *cheapNegCell;
  const StandardCell *cheapOneCell;
  const StandardCell *cheapZeroCell;

  void loadCombCell(const std::string &name);
  const StandardCell *findCheapestCell(std::vector<StandardCell> &scs);
  void findCheapestCells();
  void addSuperCell(
      const model::CellTypeID cellTypeID,
      const model::CellTypeID cellTypeIDToAdd,
      void(*func)(std::string &in, const std::string &fIn),
      std::vector<StandardCell> &scs,
      uint output);
  void addSuperCells();
  void addConstCells();

  // ReadCells
  Group *ast;
  Library library;
  TokenParser tokParser;
  ReadCellsIface *iface;
};

} // namespace eda::gate::library
