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

#include <readcells/groups.h>

#include <kitty/print.hpp>
#include <unordered_map>
#include <utility>
#include <vector>

namespace eda::gate::library {

/// Parameters for power estimation.
struct SCPinPower {
  /// TODO Cell pin fall power.
  float fallPower;
  /// TODO Cell pin rise power.
  float risePower;
};

/**
 * Desription of the standard cell read from Liberty.
*/
struct SCAttrs {
  /// Name of the standard cell.
  std::string name;
  /// Area of the standard cell.
  float area;
  /// Fall and rise powers of each input pin.
  std::vector <SCPinPower> pinPower;
  /// The number of output pins.
  std::size_t fanoutCount = 1;
};

struct DTTHash {
  std::size_t operator()(const kitty::dynamic_truth_table& dtt) const {
    std::size_t hash = 0;
    for (auto block : dtt) {
      hash ^= std::hash<uint64_t>{}(block) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }
    return hash;
  }
};

struct DTTEqual {
  bool operator()(const kitty::dynamic_truth_table& lhs,
                  const kitty::dynamic_truth_table& rhs) const {
    return kitty::equal(lhs, rhs);
  }
};

class SCLibrary final {
  using Cell = ::Cell;
  using CellID = model::CellID;
  using CellTypeID = model::CellTypeID;
  using CellTypeAttrID = model::CellTypeAttrID;
  using CellSymbol = model::CellSymbol;
  using SubnetID = model::SubnetID;

  using Attributes = std::unordered_map<SubnetID, SCAttrs>;
  using CombCells = std::vector<CellTypeID>;
  using SeqCells = std::unordered_map<CellSymbol, std::vector<CellTypeID>>;
  using TruthTables = std::unordered_map<kitty::dynamic_truth_table,
    std::vector<SubnetID>, DTTHash, DTTEqual>;

public:
  SCLibrary();
  std::vector<SubnetID> &calcPatterns();

  std::vector<SubnetID> getSubnetID(const kitty::dynamic_truth_table &tt) const;
  const SCAttrs &getCellAttrs(const SubnetID id) const;

  CombCells &getCombCells() {
    return combCells;
  }

  SeqCells &getSeqCells() {
    return seqCells;
  }

private:
  CombCells combCells;
  SeqCells  seqCells;
  std::vector<SubnetID> subnets;
  std::vector<SubnetID> patterns;
  TruthTables truthTables;
  Attributes attributes;

  bool isFunctionallyComplete();

  void addCell(const Cell &cell);

  bool isCombCell(const Cell &cell, const std::vector<std::string> &inputs,
                  const std::vector<std::string> &outputs,
                  const std::vector<std::string> &funcs) const;

  void calcPower(CellTypeID cellTypeID, std::vector<SCPinPower> &power);
  void calcPermutations(CellTypeID cellTypeID, std::vector<SCPinPower> &power);
  void addCombCell(const std::string &name,
                   const std::vector<std::string> &inputs,
                   const std::string &func,
                   const CellTypeAttrID cellTypeAttrID);

  /*void addSeqCell(const Cell &cell,
                    const std::vector<std::string> &inputs,
                    const std::string &func,
                    model::CellTypeAttrID cellTypeAttrID);*/
};
} // namespace eda::gate::library