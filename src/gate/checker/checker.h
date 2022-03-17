//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gate.h"

using namespace eda::gate::model;

namespace eda::gate::checker {

/**
 * \brief Implements a logic equivalence checker (LEC).
 * \author <a href="mailto:kamkin@ispras.ru">Alexander Kamkin</a>
 */
class Checker final {
public:
  /// Checks logic equivalence of two netlists.
  bool check(const Netlist &lhs, const Netlist &rhs);
};

} // namespace eda::gate::checker
