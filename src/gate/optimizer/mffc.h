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

/** @brief Returns a view of fanout-free cone limited by cut.
 *  @param builder The builder of a subnet.
 *  @param rootID The root ID.
 *  @param cut The nodes of the cut that limit MFFC.
 *  @return The subnet view of MFFC.
 */
model::SubnetView getMffc(model::SubnetBuilder &builder, size_t rootID,
                          const std::vector<size_t> &cut);

/** @brief Returns a view of fanout-free cone limited by inputs of view.
 *  @param builoder The builder of a subnet.
 *  @param view The view according to which MFFC is built.
 *  @return The subnet view of MFFC.
 */
model::SubnetView getMffc(model::SubnetBuilder &builder,
                          const model::SubnetView &view);

/** @brief Returns a view of fanout-free cone limited by it max depth.
 *  @param subnetBuilder The builder of a subnet.
 *  @param rootID The root ID.
 *  @param maxDepth Max distance from the root to inputs of MFFC.
 *  @return The subnet view of MFFC.
 */
model::SubnetView getMffc(model::SubnetBuilder &builder, size_t rootID,
                          size_t maxDepth);

/** @brief Returns a view of maximum fanout-free cone.
 *  @param builder The builder of a subnet.
 *  @param rootID The root ID.
 *  @return The subnet view of MFFC.
 */
model::SubnetView getMffc(model::SubnetBuilder &builder, size_t rootID);

} // namespace eda::gate::optimizer
