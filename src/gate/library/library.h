//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/library/library_types.h"

#include <string>

namespace eda::gate::library {

class SCLibrary final {
public:
  friend class SCLibraryFactory;

  struct SCLibraryProperties {
    uint maxArity = 0;
    const StandardCell *cheapNegCell = nullptr;
    const StandardCell *cheapOneCell = nullptr;
    const StandardCell *cheapZeroCell = nullptr;  
  };

  //movable, bot not copyable
  SCLibrary (SCLibrary &&) = default;
  SCLibrary &  operator= (SCLibrary &&) = default;
  SCLibrary (const SCLibrary &) = delete;
  SCLibrary & operator= (const SCLibrary &) = delete;

  void addCells(std::vector<StandardCell> &&cells);
  void addTemplates(std::vector<LutTemplate> &&templates);

  //Accesors for existing methods start
  const StandardCell* getCellPtr(const model::CellTypeID &cellTypeID) const;

  //Accesors for existing methods end
  const std::vector<StandardCell>& getCombCells() const {
    return combCells_;
  }
  const std::vector<LutTemplate> & getTemplates() const {
    return templates_;
  };
  const SCLibraryProperties& getProperties() const {
    return properties_;
  };

private:

  SCLibrary() = default;

  void internalLoadCombCell(StandardCell &&cell);
  const StandardCell *findCheapestCell(
    const std::vector<StandardCell> &scs) const;
  void findCheapestCells();
  void addSuperCell(
      const StandardCell &cellSrc,
      const StandardCell &cellToAdd,
      void(*func)(std::string &in, const std::string &fIn),
      std::vector<StandardCell> &scs,
      uint output);

  void addSuperCells();
  void fillSearchMap();

  SCLibraryProperties properties_;

  std::vector<WireLoadModel> wires_;
  std::vector<LutTemplate> templates_;
  std::vector<StandardCell> combCells_;
  std::vector<StandardCell> negCombCells_;
  std::vector<StandardCell> constOneCells_;
  std::vector<StandardCell> constZeroCells_;

  std::unordered_map<model::CellTypeID, const StandardCell*> searchMap_;
};

} // namespace eda::gate::library
