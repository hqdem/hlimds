//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <map>
#include <memory>

#include "kitty/constructors.hpp"

#include "gate/optimizer2/resynthesis/unitized_table.h"
#include "gate/optimizer2/synthesizer.h"

namespace eda::gate::optimizer2::resynthesis {

/**
 * \brief Implements an Akers method of resynthesis.
 * 
 * The algorithm based on the article "Synthesis of combinational logic using
 * three-input majority gates" by Sheldon B. Akers, Jr. (1962).
*/
class AkersAlgorithm : public Synthesizer<kitty::dynamic_truth_table> {

public:

  //===--------------------------------------------------------------------===//
  // Types
  //===--------------------------------------------------------------------===//

  using Arguments = std::set<unsigned>;
  using ArgumentsSet = std::set<Arguments>;
  using CanditateList = std::map<std::set<unsigned>, std::vector<unsigned>>;
  using ColumnsToRemove = std::vector<unsigned>;
  using EssentialEdge = std::unordered_map<unsigned,
      std::vector<std::pair<uint32_t, uint32_t>>>;
  using RowNums = std::unordered_set<uint32_t>;
  using Subnet = eda::gate::model::Subnet;
  using SubnetBuilder = eda::gate::model::SubnetBuilder;
  using SubnetID = eda::gate::model::SubnetID;
  using TruthTable = kitty::dynamic_truth_table;

  //===--------------------------------------------------------------------===//
  // Constructors/Destructors
  //===--------------------------------------------------------------------===//

  /// Empty constructor.
  AkersAlgorithm() {}

  /// Constructs unitized table of "func" without the sets where "care" has 0.
  AkersAlgorithm(const TruthTable &func, const TruthTable &care);

  //===--------------------------------------------------------------------===//
  // Convenience Methods
  //===--------------------------------------------------------------------===//

  /// Resynthesize.
  SubnetID synthesize(const TruthTable &func) override;

  /// Launch the Akers algorithm.
  SubnetID run();

  /// Returns number of MAJ gates that were added by the algorithm.
  uint64_t getNMaj() {
    return nMaj;
  }

private:

  //===--------------------------------------------------------------------===//
  // Internal Methods
  //===--------------------------------------------------------------------===//

  /// Adds a majority gate.
  void addMajGate(const Arguments &gate);

  /**
   * Finds the best set of arguments for a majority gate.
   * @param columnsToRemove For writing numbers of columns that may be removed.
   * @return Set of arguments for MAJ that may remove columnsToRemove.
   */
  Arguments findBestGate(ColumnsToRemove &columnsToRemove);

  /// Chooses one gate from the list of found gates.
  Arguments chooseGate(Arguments &candidate, ColumnsToRemove &forRemoval,
                       const CanditateList &gates,
                       ColumnsToRemove &columnsToRemove);
  
  /// Tries to find the best gate to remove N (2 <= N <= 3) columns.
  Arguments findEliminatingNColsGate(CanditateList &gates,
                                     ColumnsToRemove &columnsToRemove,
                                     const unsigned n);

  /// Returns best set of arguments that was found by others functions.
  Arguments setWhatFound(const Arguments &args,
                         const ColumnsToRemove &forRemoval,
                         ColumnsToRemove &columnsToRemove);

  /**
   * Finds sets of arguments for a majority gate
   * that lead to removal of the particular column.
   * @param essentialRows Numbers of "essential" rows in the column for erasing.
   * @param index The number of the column for removing.
   * @return Set of sets of possible arguments.
   */
  ArgumentsSet findGatesForColumnRemoval(const RowNums &essentialRows, 
                                         unsigned index);

  /**
   * Finds set of arguments for a majority gate
   * that leads to the best decrease of the number of "essential" ones.
   */
  Arguments findEliminatingOnesGate();

  /**
   * Checks whether it is possible to remove rows after adding MAJ(args).
   * @param args Arguments for a MAJ gate.
   * @param colsToRemove Numbers of columns for removing after adding MAJ.
   * @return Whether this removal leads to a deletion of rows.
   */
  bool mayDeleteRows(const Arguments &args,
                     const ColumnsToRemove &colsToRemove);

  /**
   * Counts number of "essential" ones
   * that will lose this property after adding MAJ(c1, c2, c3).
   */
  uint64_t countRemovedOnes(unsigned c1, unsigned c2, unsigned c3);

  /// Increments the counter of essential ones that may be eliminated.
  void incCounter(uint64_t &counter, RowNums &toRemove, uint32_t rowNum);

  /// Decrements the counter of essential ones that may be eliminated.
  void decCounter(uint64_t &counter, RowNums &cantRemove,
                  RowNums &toRemove, uint32_t rowNum);
  
  //===--------------------------------------------------------------------===//
  // Internal Fields
  //===--------------------------------------------------------------------===//

  /// Unitized table.
  UnitizedTable table;

  /// Number of variables of the input function.
  uint32_t nVariables;

  /// Vector of id of cells of the subnet.
  std::vector<size_t> idx;

  /// The ID of the cell ONE
  size_t oneId = 0;

  /// The ID of the cell ZERO
  size_t zeroId = 0;

  /// Builder for the subnet.
  SubnetBuilder subnetBuilder;

  /// Counts how many times in a row findEliminatingOnesGate() was called.
  unsigned nCallElimFunc = 0;

  /// A columns number before launching a function for reducing essential ones.
  unsigned nInnerColumns = 0;

  /// Pairs of graph nodes or pairs of "essential" positions of ones.
  EssentialEdge pairEssentialRows;

  /// The number of MAJ(x, y, z) gates in the GNet.
  uint64_t nMaj = 0;
};

} // namespace eda::gate::optimizer2::resynthesis
