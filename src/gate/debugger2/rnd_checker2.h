//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once
#include "base_checker2.h"
#include "gate/simulator2/simulator.h"
#include "miter2.h"
#include "util/logging.h"

#include <cassert>
#include <cmath>

namespace eda::gate::debugger2 {

using Simulator = simulator2::Simulator;
using CheckerResult = eda::gate::debugger2::CheckerResult;

Simulator::DataVector getAllValues(size_t nIn, size_t count);

/// Checks the equivalence of the specified nets using simulation.
class RndChecker2 : public BaseChecker2, public util::Singleton<RndChecker2> {
friend class util::Singleton<RndChecker2>;

public:
  /**
   * @copydoc BaseChecker2::equivalent
   */

  CheckerResult equivalent(const Subnet &lhs,
                           const Subnet &rhs,
                           const CellToCell &gmap) const override;
  /// Sets the number of random values checked, if the check is inexhaustive.
  void setTries(int tries);
  /**
   * \brief Sets the mode of the check.
   * @param exhaustive True, if all possible input values are to be simulated,
   * false otherwise.
   */
  void setExhaustive(bool exhaustive);

private:
  RndChecker2(bool exhaustive = true, int tries = 0) {
    this->exhaustive = exhaustive;
    this->tries = tries;
  }
  std::uint64_t tries;
  bool exhaustive;
};

} // namespace eda::gate::debugger2
