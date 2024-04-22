//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer2/safe_passer.h"
#include "gate/optimizer2/subnet_iterator.h"

namespace eda::gate::optimizer2 {

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
  using SafePasser = eda::gate::optimizer2::SafePasser;
  using Subnet     = eda::gate::model::Subnet;
  /// @endcond

  /**
   * @brief Сonstructor from subnet builder.
   * @param subnetBuilder Subnet for iteration.
   * @param nIn The number of inputs for SubnetFragment.
   */
  AreaSubnetIterator(const SubnetBuilder &subnetBuilder,
                     SafePasser &iter,
                     uint16_t nIn)
      : SubnetIteratorBase(subnetBuilder), iter(iter), nIn(nIn)
  {}

  SubnetFragment next() override;

private:
  SafePasser &iter;
  uint16_t nIn;
};

} // namespace eda::gate::optimizer2
