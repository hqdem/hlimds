//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"
#include "gate/model/subnetview.h"

#include <vector>

namespace eda::gate::optimizer {

/// Returns a view of reconvergence-driven cut based on the given builder.
model::SubnetView getReconvergentCut(
    model::SubnetBuilder &builder,
    const std::vector<model::EntryID> &roots,
    uint16_t cutSize);

/// Returns a view of reconvergence-driven cut based on the given builder.
inline model::SubnetView getReconvergentCut(
    model::SubnetBuilder &builder,
    model::EntryID root,
    uint16_t cutSize) {

  return getReconvergentCut(
      builder, std::vector<model::EntryID>{root}, cutSize);
}

} // namespace eda::gate::optimizer
