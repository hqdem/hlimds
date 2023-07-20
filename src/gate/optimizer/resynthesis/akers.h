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

#include "gate/model/gnet.h"
#include "gate/optimizer/resynthesis/unitized_table.h"

namespace eda::gate::optimizer::resynthesis {

/**
 * \brief Implements an Akers method of resynthesis.
 * 
 * The algorithm based on the article "Synthesis of combinational logic using
 *  three-input majority gates" by Sheldon B. Akers, Jr. (1962).
*/
class AkersAlgorithm {

public:

  //===--------------------------------------------------------------------===//
  // Types
  //===--------------------------------------------------------------------===//
  
  using Arguments = std::set<unsigned>;
  using ArgumentsSet = std::set<Arguments>;
  using CanditateList = std::map<std::set<unsigned>, std::vector<unsigned>>;
  using ColumnsToRemove = std::vector<unsigned>;
  using Gate = eda::gate::model::Gate;
  using GNet = eda::gate::model::GNet;
  using SignalList = Gate::SignalList;
  using TruthTable = kitty::dynamic_truth_table;

  //===--------------------------------------------------------------------===//
  // Constructors/Destructors
  //===--------------------------------------------------------------------===//

  /// Constructs unitized table of "func" without the sets where "care" has 0.
  AkersAlgorithm(const TruthTable &func, const TruthTable &care);

  //===--------------------------------------------------------------------===//
  // Convenience Methods
  //===--------------------------------------------------------------------===//

  /** Launch the Akers algorithm.
   *  @param inputs For writing signals of gates In().
   *  @param outputId For writing Id of gate Out().
   *  @return Shared pointer of GNet.
   */
  std::shared_ptr<GNet> run(SignalList &inputs, Gate::Id &outputId);

private:

  //===--------------------------------------------------------------------===//
  // Internal Methods
  //===--------------------------------------------------------------------===//

  /// Adds a majority gate.
  void addMajGate(const Arguments &gate);

  /** Finds the best set of arguments for a majority gate.
   *  @param columnsToRemove For writing numbers of columns that may be removed.
   *  @return Set of arguments for MAJ that may remove columnsToRemove.
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

  /** Finds sets of arguments for a majority gate
   *  that lead to removal of the particular column.
   *  @param essentialRows Numbers of essential rows in the column for removing.
   *  @param index The number of the column for removing.
   *  @return Set of sets of possible arguments.
   */
  ArgumentsSet findGatesForColumnRemoval
    (const std::set<uint32_t> &essentialRows, unsigned index);

  /** Finds set of arguments for a majority gate
   * that leads to the best decrease of the number of "essential" ones.
   */
  Arguments findEliminatingOnesGate();

  /** Checks whether it is possible to remove rows after adding MAJ(args).
   *  @param args Arguments for a MAJ gate.
   *  @param colsToRemove Numbers of columns for removing after adding MAJ.
   *  @return Whether this removal leads to a deletion of rows.
   */
  bool mayDeleteRows(const Arguments &args,
                     const ColumnsToRemove &colsToRemove);

  //===--------------------------------------------------------------------===//
  // Internal Fields
  //===--------------------------------------------------------------------===//

  /// Unitized table.
  UnitizedTable table;

  /// Number of variables of the input function.
  uint32_t nVariables;

  /// List of input and other signals for the net.
  Gate::SignalList signals;

  /// Net that implement the input function.
  std::shared_ptr<GNet> net;

  /// Counts how many times in a row findEliminatingOnesGate() was called.
  unsigned nCallElimFunc = 0;

  /// The number of columns before the launching 
  /// findEssentialOnesEliminatingGate().
  unsigned nInnerColumns = 0;
};
} // namespace eda::gate::optimizer::resynthesis
