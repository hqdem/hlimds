//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"
#include "gate/optimizer/reconvergence_cut.h"

#include <unordered_map>
#include <unordered_set>

namespace eda::gate::optimizer {

/** @brief Returns maximum fanout-free cone limited by cut.
 *  @param subnetBuilder A builder of a subnet.
 *  @param root A root ID.
 *  @param leaves Nodes of the cut that limit a cone.
 *  @param map The mapping from cone to SubnetBuilder.
 *  @return A cone with an input/output map.
 */
model::SubnetID getMffc(model::SubnetBuilder &builder, size_t root,
                        const std::vector<size_t> &leaves,
                        std::unordered_map<size_t, size_t> &map);

} // namespace eda::gate::optimizer
