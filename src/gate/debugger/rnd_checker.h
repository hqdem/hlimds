//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once
#include "base_checker.h"
#include "checker.h"
#include "gate/model/gnet.h"
#include "gate/simulator/simulator.h"
#include "miter.h"
#include "rtl/library/flibrary.h"
#include "util/logging.h"

#include <cassert>
#include <cmath>

using GNet = eda::gate::model::GNet;

namespace eda::gate::debugger {

/**
 *  \brief Goes through values and checks miter output.
 *  @param miter Miter which will receive values.
 *  @param tries Number of random values checked, if the check is inexhaustive.
 *  @param exhaustive Sets the mode of the check.
 *  @return The result of the check.
 */
CheckerResult rndChecker(GNet &miter,
                         const unsigned int tries,
                         const bool exhaustive);

class RndChecker : public BaseChecker, public util::Singleton<RndChecker> {
friend class util::Singleton<RndChecker>;

public:
  /**
 *  @copydoc base_checker.h:equivalent
 */
  CheckerResult equivalent(GNet &lhs,
                           GNet &rhs,
                           Checker::GateIdMap &gmap) override;
  void setTries(int tries);
  void setExhaustive(bool exhaustive);
  RndChecker(bool exhaustive = true, int tries = 0){
    this->exhaustive = exhaustive;
    this->tries = tries;
  }
private:
  int tries;
  bool exhaustive;
};

} // namespace eda::gate::debugger
