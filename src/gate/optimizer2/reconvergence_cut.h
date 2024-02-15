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
size_t computeCost(const model::Subnet &subnet,
                   uint32_t idx,
                   const std::unordered_set<uint32_t> &visited);

/// Returns a reconvergence-driven cut.
std::vector<uint32_t> getReconvergenceCut(const model::Subnet &subnet,
                                          const std::vector<uint32_t> &roots,
                                          size_t cutSize);

/// Returns a reconvergence-driven cut.
inline std::vector<uint32_t> getReconvergenceCut(const model::Subnet &subnet,
                                                 uint32_t root,
                                                 size_t cutSize) {

  return getReconvergenceCut(subnet, std::vector<uint32_t>{root}, cutSize);
}

} // namespace eda::gate::optimizer2
