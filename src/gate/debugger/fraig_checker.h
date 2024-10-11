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
#include <vector>

namespace eda::gate::debugger {

using SimValuesStorage = std::vector<std::bitset<64>>;

/**
 * \brief Implements FRAIG-based method of LEC.
 *
 * The algorithm is based on the article "Improvements to combinational
 * equivalence checking" by A. Mishchenko, S. Chatterjee, R. Brayton (2006).
 */
class FraigChecker final : public BaseChecker,
                           public util::Singleton<FraigChecker> {
  friend class util::Singleton<FraigChecker>;

public:
  /// @copydoc BaseChecker::isSat
  CheckerResult isSat(const model::Subnet &subnet) const override;

  /// Simulator arity limit
  static constexpr size_t simLimit = 64;

  /// Cell comparisons limit
  static constexpr size_t compareLimit = 500;

private:
  FraigChecker() {}
};

} // namespace eda::gate::debugger
