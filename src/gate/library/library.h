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
#include <unordered_set>

namespace eda::gate::library {

class SCLibrary final {
public:
  friend class SCLibraryFactory;

  struct SCLibraryProperties {
    uint maxArity = 0;
    std::optional<WireLoadSelection> wlmSelection;
    const WireLoadModel* defaultWLM = nullptr;
    const StandardCell *cheapNegCell = nullptr;
    const StandardCell *cheapOneCell = nullptr;
    const StandardCell *cheapZeroCell = nullptr;  
  };

  //movable, bot not copyable
  SCLibrary (SCLibrary &&) = default;
  SCLibrary &  operator= (SCLibrary &&) = default;
  SCLibrary (const SCLibrary &) = delete;
  SCLibrary & operator= (const SCLibrary &) = delete;

  bool addCells(std::vector<StandardCell> &&cells);
  bool addTemplates(std::vector<LutTemplate> &&templates);
  bool addWLMs(std::vector<WireLoadModel> &&wlms);
  bool addProperties(const std::string &defaultWLMsName,
                     WireLoadSelection &&selection);

  //Accesors for existing methods start
  const StandardCell* getCellPtr(const model::CellTypeID &cellTypeID) const;

  //Accesors for existing methods end
  const std::vector<StandardCell>& getCombCells() const {
    return combCells_;
  }
  const std::vector<WireLoadModel> & getWLMs() const {
    return wires_;
  };
  const std::vector<LutTemplate> & getTemplates() const {
    return templates_;
  };
  const SCLibraryProperties & getProperties() const {
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

  struct CollisionMaps{
    std::unordered_set<std::string> cellNames;
    std::unordered_set<std::string> templateNames;
    std::unordered_set<std::string> wlmNames;
  };

  bool checkCellCollisions(const std::vector<StandardCell> &cells);
  bool checkTemplateCollisions(const std::vector<LutTemplate> &templates);
  bool checkWLMCollisions(const std::vector<WireLoadModel> &wlms);

  SCLibraryProperties properties_;
  CollisionMaps collisions_;

  std::vector<WireLoadModel> wires_;
  std::vector<LutTemplate> templates_;
  std::vector<StandardCell> combCells_;
  std::vector<StandardCell> negCombCells_;
  std::vector<StandardCell> constOneCells_;
  std::vector<StandardCell> constZeroCells_;

  std::unordered_map<model::CellTypeID, const StandardCell*> searchMap_;
};

} // namespace eda::gate::library
