//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"

#include <unordered_map>
#include <vector>

namespace eda::gate::optimizer {

/// Returns a reconvergence-driven cut based on the given subnet builder.
std::vector<size_t> getReconvergenceCut(
    model::SubnetBuilder &builder,
    const std::vector<size_t> &roots,
    uint16_t cutSize);

/// Returns a reconvergence-driven cut based on the given subnet builder.
inline std::vector<size_t> getReconvergenceCut(
    model::SubnetBuilder &builder,
    size_t root,
    uint16_t cutSize) {

  return getReconvergenceCut(builder, std::vector<size_t>{root}, cutSize);
}

/// Returns a reconvergence-driven window based on the given subnet builder.
model::SubnetID getReconvergenceWindow(
    model::SubnetBuilder &builder,
    const std::vector<size_t> &roots,
    uint16_t cutSize,
    std::unordered_map<size_t, size_t> &map);

/// Returns a reconvergence-driven cone based on the given subnet builder.
inline model::SubnetID getReconvergenceCone(
    model::SubnetBuilder &builder,
    size_t root,
    uint16_t cutSize,
    std::unordered_map<size_t, size_t> &map) {

  return getReconvergenceWindow(
      builder, std::vector<size_t>{root}, cutSize, map);
}

} // namespace eda::gate::optimizer
