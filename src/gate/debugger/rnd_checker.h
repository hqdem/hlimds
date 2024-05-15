//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/debugger/base_checker.h"
#include "gate/simulator2/simulator.h"
#include "util/logging.h"

#include <cassert>
#include <cmath>

namespace eda::gate::debugger {

using Simulator = simulator2::Simulator;
using CheckerResult = eda::gate::debugger::CheckerResult;

Simulator::DataVector getAllValues(size_t nIn, size_t count);

/// Checks the equivalence of the specified nets using simulation.
class RndChecker : public BaseChecker, public util::Singleton<RndChecker> {
friend class util::Singleton<RndChecker>;

public:
  /**
   * @copydoc BaseChecker::isSat
   */

  CheckerResult isSat(const SubnetID id) const override;

  /// Sets the number of random values checked, if the check is inexhaustive.
  void setTries(int tries) { this->tries = tries; }

  /**
   * \brief Sets the mode of the check.
   * @param exhaustive True, if all possible input values are to be simulated,
   * false otherwise.
   */
  void setExhaustive(bool exhaustive) { this->exhaustive = exhaustive; }

private:
  RndChecker(bool exhaustive = true, unsigned tries = 0) {
    this->exhaustive = exhaustive;
    this->tries = tries;
  }
  unsigned tries;
  bool exhaustive;
};

} // namespace eda::gate::debugger
