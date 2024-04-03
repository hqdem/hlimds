//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/debugger2/base_checker2.h"
#include "gate/debugger2/miter2.h"
#include "gate/simulator2/simulator.h"
#include "util/logging.h"

#include <optional>
#include <random>
#include <unordered_map>

namespace eda::gate::debugger2 {

using CellToCell = eda::gate::debugger2::CellToCell;
using LecType = eda::gate::debugger2::options::LecType;
using SubnetID = eda::gate::model::SubnetID;
using Simulator = eda::gate::simulator2::Simulator;

/**
 * \brief Checks the equivalence of the specified subnets using SAT-solver.
 *
 * The checking method is based on the article "Scalable and Scalably-Verifiable
 * Sequential Synthesis" by A. Mishchenko (2008).
 */

class SeqChecker : public BaseChecker2 {
public:

  /**
   * @copydoc BaseChecker2::equivalent
   */
  CheckerResult equivalent(const Subnet &lhs, const Subnet &rhs,
                           const CellToCell &cmap) const override;

  // Sets the number of attempts to find registers stuck at a constant
  void setSimulateTries(int tries);

  // Sets the number of attempts to 1 and the seed used during the simulation
  void setSimulateSeed(uint32_t s);

private:
  // Returns checker result based on the elements left in subnet
  CheckerResult getResult(const Subnet &subnet) const;

  int nsimulate = 10;
  bool setSeed = false;
  uint32_t seed;
};

void info(const Subnet &subnet, const std::string = "");

// Returns Subnet without hanging cells
const Subnet &seqSweep(const Subnet &subnet);

// Merges equivalence classes into representative class
const Subnet &merge(const Subnet &subnet,
                    std::unordered_map<size_t, std::vector<size_t>> &classes,
                    bool speculative = false);

// Merges classes into constant(ZERO/ONE)
const Subnet &merge(const Subnet &subnet, CellSymbol symbol,
                    const std::vector<size_t> &ids);

// Swaps values defined in ids between Datavectors
void swapFlipsValues(const Simulator::DataVector &vals1,
                     Simulator::DataVector &vals2,
                     const std::vector<std::pair<size_t, size_t>> &pairs);

// Gets flip flop IDs from subnet
void getFlipsIds(
    const Subnet &subnet,
    std::unordered_map<uint32_t, std::pair<size_t, size_t>> &flips);

// Returns Subnet without equivalent registers and without registers stuck at a
// constant
const Subnet &structuralRegisterSweep(const Subnet &subnet, const int nsimulate,
                                      bool setSeed, uint32_t seed);

} // namespace eda::gate::debugger2
