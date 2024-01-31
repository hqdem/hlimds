//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once
#include "base_checker.h"
#include "gate/simulator/simulator.h"
#include "miter.h"

#include <cmath>

using GNet = eda::gate::model::GNet;
using GateIdMap = eda::gate::debugger::SatChecker::GateIdMap;

namespace eda::gate::debugger {

/// Checks the equivalence of the specified nets using simulation.
class RndChecker : public BaseChecker, public util::Singleton<RndChecker> {
friend class util::Singleton<RndChecker>;

public:
  /**
   * @copydoc BaseChecker::equivalent
   */
  CheckerResult equivalent(const GNet &lhs,
                           const GNet &rhs,
                           const GateIdMap &gmap) const override;
  /// Sets the number of random values checked, if the check is inexhaustive.
  void setTries(int tries);
  /**
   * \brief Sets the mode of the check.
   * @param exhaustive True, if all possible input values are to be simulated,
   * false otherwise.
   */
  void setExhaustive(bool exhaustive);

private:
  RndChecker(bool exhaustive = true, int tries = 0) {
    this->exhaustive = exhaustive;
    this->tries = tries;
  }

  int tries;
  bool exhaustive;
};

} // namespace eda::gate::debugger
