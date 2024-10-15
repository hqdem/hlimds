//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/library/library_types.h"
#include "util/truth_table.h" //TODO: try to move to source

#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace eda::gate::library {

class SCLibrary final {
public:
  friend class SCLibraryFactory;

  struct SCLibraryProperties {
    uint maxArity = 0;
    std::optional<WireLoadSelection> wlmSelection;
    const WireLoadModel* defaultWLM = nullptr;
    std::pair<const StandardCell*, size_t> cheapNegCell = {nullptr,-1};
    std::pair<const StandardCell*, size_t> cheapOneCell = {nullptr,-1};
    std::pair<const StandardCell*, size_t> cheapZeroCell = {nullptr,-1};
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
  void prepareLib() {
    if (!libPrepared) {
      findCheapestCells();
      addSuperCells();
      addConstCells();
      completePclasses();
      updateProperties();
      fillSearchMap();
      libPrepared = true;
    }
  };

  // TODO: should be moved somewhere else
  struct CanonInfo {
    util::TruthTable ctt;
    util::NpnTransformation transform;
  };

private:

  SCLibrary() = default;

  void internalLoadCombCell(StandardCell &&cell);
   std::pair<const StandardCell*, size_t> findCheapestCell(
    const std::vector< std::pair<StandardCell, size_t>> &scs) const;
  void findCheapestCells();
  void addSuperCell(
      const StandardCell &cellSrc,
      const StandardCell &cellToAdd,
      std::vector<StandardCell> &scs,
      uint output);

  void addSuperCells();
  void addConstCells();
  void fillSearchMap();
  void updateProperties();

  struct CollisionMaps{
    std::unordered_set<std::string> cellNames;
    std::unordered_set<std::string> templateNames;
    std::unordered_set<std::string> wlmNames;
  };

  bool checkCellCollisions(const std::vector<StandardCell> &cells);
  bool checkTemplateCollisions(const std::vector<LutTemplate> &templates);
  bool checkWLMCollisions(const std::vector<WireLoadModel> &wlms);


  using CellLogPair = std::pair<const StandardCell*, uint16_t>;
  using CttMap = std::unordered_map<util::TruthTable, std::vector<CellLogPair>>;

  void completePclasses();
  void completeP1classes(CttMap &existingCttP1);
  void completeP2classes(CttMap &existingCttP2);
  void completeP3classes(CttMap &existingCttP3, CttMap &existingCttP2);
  CellLogPair buildP2CellForF3MiniTerm(uint8_t miniTermF3, CttMap &existingCttP2);
  CellLogPair buildF3termEquivalentCell(
    const std::vector<CanonInfo> &termCanons,
    const std::vector<CellLogPair> &cellEquivalents,
    CttMap &existingCttP2);

  std::pair<CellLogPair, CellLogPair> createP2AndOR(CttMap &existingCttP2, const CellLogPair &exCellInv);
  CellLogPair buildP2CellForF2MiniTerm(
    uint8_t miniTermF2, 
    const CellLogPair &cellAndP,
    const CellLogPair &cellInvP);
  CellLogPair buildF2termEquivalentCell(
    const std::vector<CanonInfo> &termCanons,
    const std::vector<CellLogPair> &cellEquivalents,
    const CellLogPair &exCellOr);
  CellLogPair getBaseP2Term(CttMap &existingCttP2, const CellLogPair &exCellInv);
  CellLogPair addNegOutput(const CellLogPair &sourceCell, const CellLogPair &exCellInv);

  SCLibraryProperties properties_;
  CollisionMaps collisions_;

  std::vector<WireLoadModel> wires_;
  std::vector<LutTemplate> templates_;
  std::vector<StandardCell> combCells_;
  std::vector< std::pair<StandardCell, size_t>> negCombCells_;
  std::vector< std::pair<StandardCell, size_t>> constOneCells_;
  std::vector< std::pair<StandardCell, size_t>> constZeroCells_;
  bool libPrepared = false;

  //TODO: remove
  std::list<StandardCell> pComplCells_;

  std::unordered_map<model::CellTypeID, const StandardCell*> searchMap_;
};

} // namespace eda::gate::library