//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer2/subnet_iterator.h"

namespace eda::gate::optimizer2 {

/**
 * @brief An iterator over the subnet for the area optimization.
 */
class AreaSubnetIterator : public SubnetIteratorBase {
public:

  /// @cond ALIASES
  using Cell      = eda::gate::model::Subnet::Cell;
  using EntryIter = eda::gate::model::EntryIterator;
  using IdxMap    = std::unordered_map<size_t, size_t>;
  using Link      = eda::gate::model::Subnet::Link;
  using LinkList  = eda::gate::model::Subnet::LinkList;
  using Subnet    = eda::gate::model::Subnet;
  /// @endcond

  /**
   * @brief Сonstructor from subnet builder.
   * @param subnetBuilder Subnet for iteration.
   * @param nIn The number of inputs for SubnetFragment.
   */
  AreaSubnetIterator(const SubnetBuilder &subnetBuilder, uint16_t nIn)
      : SubnetIteratorBase(subnetBuilder), nIn(nIn), iter(subnetBuilder.begin())
  {}

  SubnetFragment next() override;

private:
  uint16_t nIn;
  EntryIter iter;
};

} // namespace eda::gate::optimizer2
