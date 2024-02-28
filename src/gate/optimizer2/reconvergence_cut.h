//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"

#include <unordered_set>
#include <vector>

namespace eda::gate::optimizer2 {

/// Returns the cost of a node.
unsigned computeCost(const model::Subnet &subnet,
                     size_t idx,
                     const std::unordered_set<size_t> &visited);

/// Returns a reconvergence-driven cut.
std::vector<size_t> getReconvergenceCut(const model::Subnet &subnet,
                                        const std::vector<size_t> &roots,
                                        size_t cutSize);

/// Returns a reconvergence-driven cut.
inline std::vector<size_t> getReconvergenceCut(const model::Subnet &subnet,
                                               size_t root,
                                               size_t cutSize) {

  return getReconvergenceCut(subnet, std::vector<size_t>{root}, cutSize);
}

} // namespace eda::gate::optimizer2
