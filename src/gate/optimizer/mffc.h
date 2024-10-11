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
#include "gate/optimizer/reconvergence.h"

namespace eda::gate::optimizer {

/// @brief Returns a view of fanout-free cone limited by cut.
model::SubnetView getMffc(const std::shared_ptr<model::SubnetBuilder> &builder,
                          model::EntryID rootID,
                          const model::EntryIDList &cut);

/// @brief Returns a view of fanout-free cone limited by inputs of view.
model::SubnetView getMffc(const std::shared_ptr<model::SubnetBuilder> &builder,
                          const model::SubnetView &view);

/// @brief Returns a view of fanout-free cone limited by it max depth.
model::SubnetView getMffc(const std::shared_ptr<model::SubnetBuilder> &builder,
                          model::EntryID rootID,
                          uint32_t maxDepth);

/// @brief Returns a view of maximum fanout-free cone.
model::SubnetView getMffc(const std::shared_ptr<model::SubnetBuilder> &builder,
                          model::EntryID rootID);

} // namespace eda::gate::optimizer
