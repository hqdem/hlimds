//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
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

Simulator::DV getAllValues(size_t nIn, size_t count);

class RndChecker2 : public BaseChecker2, public util::Singleton<RndChecker2> {
friend class util::Singleton<RndChecker2>;

public:
  /**
   *  @copydoc base_checker.h:equivalent
   */
  CheckerResult equivalent(Subnet &lhs,
                           Subnet &rhs,
                           CellToCell &gmap) override;
  void setTries(int tries);
  void setExhaustive(bool exhaustive);
  RndChecker2(bool exhaustive = true, int tries = 0) {
    this->exhaustive = exhaustive;
    this->tries = tries;
  }
private:
  std::uint64_t tries;
  bool exhaustive;
};

} // namespace eda::gate::debugger2
