//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/debugger/base_checker.h"
#include "gate/simulator/simulator.h"

#include <bitset>

namespace eda::gate::debugger {

using CounterEx = std::vector<std::bitset<64>>;

/**
 * \brief Implements FRAIG-based method of LEC.
 *
 * The algorithm is based on the article "Improvements to combinational
 * equivalence checking" by A. Mishchenko, S. Chatterjee, R. Brayton (2006).
 */
class FraigChecker : public BaseChecker, public util::Singleton<FraigChecker> {
friend class util::Singleton<FraigChecker>;

public:
  /**
   * @copydoc BaseChecker::isSat
   */
  CheckerResult isSat(const SubnetID id) const override;

  /// Simulator arity limit
  static const size_t simLimit = 64;
private:

  static void netSimulation(simulator::Simulator &simulator,
                            const uint16_t &nIn,
                            const CounterEx &counterEx = {});

  FraigChecker() {}
};

} // namespace eda::gate::debugger
