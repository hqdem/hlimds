//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/celltype.h"
#include "gate/model/subnet.h"

#include "kitty/kitty.hpp"

#include <cassert>
#include <vector>

namespace eda::gate::model {

/// Evaluates the truth tables for the subnet outputs.
std::vector<kitty::dynamic_truth_table> evaluate(const Subnet &subnet);

/// Evaluates the truth table for the subnet output.
inline kitty::dynamic_truth_table evaluateSingleOut(const Subnet &subnet) {
  assert(subnet.getOutNum() == 1);
  return evaluate(subnet)[0];
}

/// Returns a truth table in which care sets are marked with 1.
kitty::dynamic_truth_table computeCare(const Subnet &subnet);

} // namespace eda::gate::model
