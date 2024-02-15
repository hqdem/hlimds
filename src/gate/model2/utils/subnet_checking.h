//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"
#include "gate/model2/utils/subnet_truth_table.h"

#include "kitty/kitty.hpp"

namespace eda::gate::model::utils {

/// Checks if the truth table of 'subnet' is equal to the 'table'.
inline bool equalTruthTables(const model::Subnet &subnet,
                             const kitty::dynamic_truth_table &table) {

  return table == model::evaluateSingleOut(subnet);
}

/// Checks if arity of the cells in 'subnet' is less or equal than 'arity'.
bool checkArity(const model::Subnet &subnet, uint16_t arity);

} // namespace eda::gate::model::utils
