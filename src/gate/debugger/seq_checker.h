//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/debugger/base_checker.h"
#include "gate/simulator/simulator.h"
#include "util/logging.h"

#include <optional>
#include <random>
#include <unordered_map>

namespace eda::gate::debugger {

/**
 * \brief Checks the equivalence of the specified subnets using SAT-solver.
 *
 * The checking method is based on the article "Scalable and Scalably-Verifiable
 * Sequential Synthesis" by A. Mishchenko (2008).
 */

class SeqChecker final : public BaseChecker {
public:

  /**
   * @copydoc BaseChecker2::isSat
   */
  CheckerResult isSat(const model::Subnet &subnet) const override;

  // Sets the number of attempts to find registers stuck at a constant
  void setSimulateTries(int tries);

  // Sets the number of attempts to 1 and the seed used during the simulation
  void setSimulateSeed(uint32_t s);

private:
  // Returns checker result based on the elements left in subnet
  CheckerResult getResult(const model::Subnet &subnet) const;

  int nsimulate = 10;
  bool setSeed = false;
  uint32_t seed;
};

void info(const model::Subnet &subnet, const std::string = "");

// Returns Subnet without hanging cells
const model::Subnet &seqSweep(const model::Subnet &subnet);

// Merges equivalence classes into representative class
const model::Subnet &merge(const model::Subnet &subnet,
                           std::unordered_map<size_t, std::vector<size_t>> &classes,
                           bool speculative = false);

// Merges classes into constant(ZERO/ONE)
const model::Subnet &merge(const model::Subnet &subnet,
                           model::CellSymbol symbol,
                           const std::vector<size_t> &ids);

// Swaps values defined in ids between Datavectors
void swapFlipsValues(const simulator::Simulator::DataVector &vals1,
                     simulator::Simulator::DataVector &vals2,
                     const std::vector<std::pair<size_t, size_t>> &pairs);

// Gets flip flop IDs from subnet
void getFlipsIds(
    const model::Subnet &subnet,
    std::unordered_map<uint32_t, std::pair<size_t, size_t>> &flips);

// Returns Subnet without equivalent registers and without registers stuck at a
// constant
const model::Subnet &structuralRegisterSweep(const model::Subnet &subnet,
                                             const int nsimulate,
                                             bool setSeed,
                                             uint32_t seed);

} // namespace eda::gate::debugger
