//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/synthesis/unitized_table.h"
#include "gate/optimizer/synthesizer.h"

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace eda::gate::optimizer::synthesis {

struct ElimOnesInfo;
struct ConstantId;
struct BuildVars;
struct Candidate;

/**
 * \brief Implements the Akers method.
 * 
 * The implementation is based on the article "Synthesis of combinational logic
 * using three-input majority gates" by Sheldon B. Akers, Jr. (1962).
 */
class AkersSynthesizer : public TruthTableSynthesizer {
public:
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
  using SubnetObject  = eda::gate::model::SubnetObject;
  using TruthTable    = util::TruthTable;
  using UnitizedTable = eda::gate::optimizer::synthesis::UnitizedTable;

  /// Empty constructor.
  AkersSynthesizer() {}

  using Synthesizer::synthesize;

  /// Synthesizes a subnet.
  SubnetObject synthesize(const TruthTable &func, const TruthTable &care,
                          uint16_t maxArity = -1) const override;

private:
  /// Launches the Akers algorithm.
  SubnetObject run(const TruthTable &func, const TruthTable &care) const;

  /// Adds a majority gate.
  void addMajGate(UnitizedTable &table, BuildVars &buildVars,
                  const Arguments &gate, uint32_t nVariables,
                  ConstantId &cid) const;

  /// Finds the best set of arguments for a majority gate.
  Candidate findBestGate(UnitizedTable &table, ElimOnesInfo &onesInfo) const;

  /// Chooses one gate from the list of found gates.
  Candidate chooseGate(UnitizedTable &table, EssentialEdge &edges,
                       Candidate &candidate, const CanditateList &gates,
                       ElimOnesInfo &onesInfo) const;

  /// Tries to find the best gate to remove N (2 <= N <= 3) columns.
  Candidate findEliminatingNColsGate(UnitizedTable &table, EssentialEdge &edges,
                                     CanditateList &gates, ElimOnesInfo &info,
                                     const unsigned n) const;

  /// Returns best set of arguments that was found by others functions.
  Candidate setWhatFound(const Candidate& candidate,
                         ElimOnesInfo &onesInfo) const;

  /// Finds sets of arguments for a MAJ gate that lead to removal of the column.
  ArgumentsSet findGatesForColumnRemoval(const UnitizedTable &table,
                                         const RowNums &essentialRows,
                                         unsigned index) const;

  /**
   * Finds set of arguments for a MAJ gate
   * that leads to the best decrease of the number of "essential" ones.
   */
  Candidate findEliminatingOnesGate(const UnitizedTable &table,
                                    EssentialEdge &edges,
                                    ElimOnesInfo &onesInfo) const;

  /// Checks whether it is possible to remove rows after adding MAJ(args).
  bool mayDeleteRows(UnitizedTable &table, const Candidate& candidate) const;

  /**
   * Counts number of "essential" ones
   * that will lose this property after adding MAJ(c1, c2, c3).
   */
  uint64_t countRemoved(const UnitizedTable &table, EssentialEdge &edges,
                        unsigned c1, unsigned c2, unsigned c3) const;

  /// Increments the counter of essential ones that may be eliminated.
  void incCounter(uint64_t &counter, RowNums &toRemove, uint32_t rowNum) const;

  /// Decrements the counter of essential ones that may be eliminated.
  void decCounter(uint64_t &counter, RowNums &cantRemove,
                  RowNums &toRemove, uint32_t rowNum) const;
};

} // namespace eda::gate::optimizer::synthesis
