//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"
#include "gate/optimizer/reconvergence_cut.h"
#include "gate/optimizer/subnet_iterator.h"

#include <unordered_map>
#include <unordered_set>

namespace eda::gate::optimizer {

/** @brief Returns maximum fanout-free cone limited by cut.
 *  @param subnetBuilder A builder of a subnet.
 *  @param root A root ID.
 *  @param leaves Nodes of the cut that limit a cone.
 *  @return A cone with an input/output map.
 */
SubnetIteratorBase::SubnetFragment getMffc(
    const model::SubnetBuilder &subnetBuilder,
    size_t root,
    const std::vector<size_t> &leaves);

} // namespace eda::gate::optimizer
