//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer2/synthesis/unitized_table.h"
#include "gate/optimizer2/synthesizer.h"

#include "kitty/constructors.hpp"

#include <map>
#include <memory>

namespace eda::gate::optimizer2::synthesis {

/** 
 *  The information about the number of inner columns and
 *  the number of calls in a row the function to eliminate "essential" ones.
 */
struct ElimOnesInfo {
  /// The number of calling in a row the function to eliminate ones.
  unsigned nCall;
  /// The number of columns before launching the function to eliminate ones.
  unsigned nInner;
};

/// The IDs of constants to avoid duplicates.
struct ConstantId {
  size_t zeroId = 0;
  bool hasZero = false;

  size_t oneId = 0;
  bool hasOne = false;
};

/// The variables for building the subnet.
struct SubBuild {
  eda::gate::model::SubnetBuilder builder;
  std::vector<size_t> idx;
};

/// The information about one candidate.
struct Candidate {
  /// Numbers of columns for a MAJ gate.
  std::set<unsigned> args;
  /// Columns that may be removed after adding the MAJ(args).
  std::vector<unsigned> toRemove;
};

/**
 * \brief Implements an Akers method of synthesis.
 * 
 * The algorithm based on the article "Synthesis of combinational logic using
 * three-input majority gates" by Sheldon B. Akers, Jr. (1962).
*/
class AkersAlgorithm : public Synthesizer<kitty::dynamic_truth_table> {

public:

  //===--------------------------------------------------------------------===//
  // Types
  //===--------------------------------------------------------------------===//

  using Arguments     = std::set<unsigned>;
  using ArgumentsSet  = std::set<Arguments>;
  using CanditateList = std::map<std::set<unsigned>, std::vector<unsigned>>;
  using Columns       = std::vector<unsigned>;
  using EssentialEdge =
    std::unordered_map<unsigned, std::vector<std::pair<uint32_t, uint32_t>>>;
  using RowNums       = std::unordered_set<uint32_t>;
  using Subnet        = eda::gate::model::Subnet;
  using SubnetBuilder = eda::gate::model::SubnetBuilder;
  using SubnetID      = eda::gate::model::SubnetID;
  using TruthTable    = kitty::dynamic_truth_table;
  using UnitizedTable = eda::gate::optimizer2::synthesis::UnitizedTable;

  //===--------------------------------------------------------------------===//
  // Constructors/Destructors
  //===--------------------------------------------------------------------===//

  /// Empty constructor.
  AkersAlgorithm() {}

  //===--------------------------------------------------------------------===//
  // Synthesize Methods
  //===--------------------------------------------------------------------===//

  /// Synthesize function without "don't care" bits.
  SubnetID synthesize(const TruthTable &func, uint16_t maxArity = -1) override;

  /// Synthesize function with "don't care" bits.
  SubnetID synthesize(const TruthTable &func, const TruthTable &care) {
    return run(func, care);
  }

private:

  //===--------------------------------------------------------------------===//
  // Internal Methods
  //===--------------------------------------------------------------------===//

  /// Launch the Akers algorithm.
  SubnetID run(const TruthTable &func, const TruthTable &care);

  /// Adds a majority gate.
  void addMajGate(UnitizedTable &table, SubBuild &subBuild,
                  const Arguments &gate, uint32_t nVariables,
                  ConstantId &cid);

  /// Finds the best set of arguments for a majority gate.
  Candidate findBestGate(UnitizedTable &table, ElimOnesInfo &onesInfo);

  /// Chooses one gate from the list of found gates.
  Candidate chooseGate(UnitizedTable &table, EssentialEdge &edges,
                       Candidate &candidate, const CanditateList &gates,
                       ElimOnesInfo &onesInfo);

  /// Tries to find the best gate to remove N (2 <= N <= 3) columns.
  Candidate findEliminatingNColsGate(UnitizedTable &table, EssentialEdge &edges,
                                     CanditateList &gates, ElimOnesInfo &info,
                                     const unsigned n);

  /// Returns best set of arguments that was found by others functions.
  Candidate setWhatFound(const Candidate& candidate, ElimOnesInfo &onesInfo);

  /// Finds sets of arguments for a MAJ gate that lead to removal of the column.
  ArgumentsSet findGatesForColumnRemoval(const UnitizedTable &table,
                                         const RowNums &essentialRows,
                                         unsigned index);

  /**
   * Finds set of arguments for a MAJ gate
   * that leads to the best decrease of the number of "essential" ones.
   */
  Candidate findEliminatingOnesGate(const UnitizedTable &table,
                                    EssentialEdge &edges,
                                    ElimOnesInfo &onesInfo);

  /// Checks whether it is possible to remove rows after adding MAJ(args).
  bool mayDeleteRows(UnitizedTable &table, const Candidate& candidate);

  /**
   * Counts number of "essential" ones
   * that will lose this property after adding MAJ(c1, c2, c3).
   */
  uint64_t countRemoved(const UnitizedTable &table, EssentialEdge &edges,
                        unsigned c1, unsigned c2, unsigned c3);

  /// Increments the counter of essential ones that may be eliminated.
  void incCounter(uint64_t &counter, RowNums &toRemove, uint32_t rowNum);

  /// Decrements the counter of essential ones that may be eliminated.
  void decCounter(uint64_t &counter, RowNums &cantRemove,
                  RowNums &toRemove, uint32_t rowNum);
};

} // namespace eda::gate::optimizer2::synthesis
