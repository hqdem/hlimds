//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/mffc.h"
#include "gate/optimizer/safe_passer.h"
#include "gate/optimizer/subnet_iterator.h"

namespace eda::gate::optimizer {

/**
 * @brief An iterator over the subnet for the area optimization.
 */
class AreaSubnetIterator : public SubnetIteratorBase {
public:

  /// @cond ALIASES
  using Cell       = eda::gate::model::Subnet::Cell;
  using IdxMap     = std::unordered_map<size_t, size_t>;
  using Link       = eda::gate::model::Subnet::Link;
  using LinkList   = eda::gate::model::Subnet::LinkList;
  using SafePasser = eda::gate::optimizer::SafePasser;
  using Subnet     = eda::gate::model::Subnet;
  /// @endcond

  /**
   * @brief Сonstructor from subnet builder.
   * @param subnetBuilder Subnet for iteration.
   * @param passer Passer for the subnet.
   * @param nIn The number of inputs for SubnetFragment.
   */
  AreaSubnetIterator(const SubnetBuilder &subnetBuilder,
                     SafePasser &passer,
                     uint16_t nIn)
      : SubnetIteratorBase(subnetBuilder), passer(passer), nIn(nIn)
  {}

  SubnetFragment next() override {
    SubnetFragment sf;
    sf.subnetID = model::OBJ_NULL_ID;

    ++passer;
    const auto rootId = *passer;
    if (subnetBuilder.getCell(rootId).isOut()) {
      return sf;
    }

    auto leaves = getReconvergenceCut(subnetBuilder, rootId, nIn);
    sf = getMffc(subnetBuilder, rootId, leaves);

    return sf;
  }

private:
  SafePasser &passer;
  uint16_t nIn;
};

} // namespace eda::gate::optimizer
