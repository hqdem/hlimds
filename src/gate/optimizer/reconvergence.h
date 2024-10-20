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
    const std::shared_ptr<model::SubnetBuilder> &builder,
    const model::EntryIDList &roots,
    uint16_t cutSize);

/// Returns a view of reconvergence-driven cut based on the given builder.
inline model::SubnetView getReconvergentCut(
    const std::shared_ptr<model::SubnetBuilder> &builder,
    model::EntryID root,
    uint16_t cutSize) {

  return getReconvergentCut(builder, model::EntryIDList{root}, cutSize);
}

} // namespace eda::gate::optimizer
