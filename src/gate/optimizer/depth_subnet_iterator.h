//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/safe_passer.h"
#include "gate/optimizer/subnet_iterator.h"

namespace eda::gate::optimizer {

/**
 * @brief iterator for depth optimization resynthesis.
 */
class DepthSubnetIterator final : public SubnetIteratorBase {

  using SafePasser = eda::gate::optimizer::SafePasser;
  using LinkList = model::Subnet::LinkList;
  using EntryMap = std::unordered_map<size_t, size_t>;
  using Link = model::Subnet::Link;

public:
  //===--------------------------------------------------------------------===//
  // Constructors/Destructors
  //===--------------------------------------------------------------------===//

  /**
   * @brief Сonstructor from subnet builder.
   * @param subnetBuilder Subnet for iteration.
   * @param cutSize Max cut size.
   * @param maxCones Max number of cone constructions.
   */
  DepthSubnetIterator(const SubnetBuilder &subnetBuilder,
                      SafePasser &iter,
                      size_t cutSize,
                      size_t maxCones) :
    SubnetIteratorBase(subnetBuilder),
    start(iter),
    cutSize(cutSize),
    maxCones(maxCones) {}

  //===--------------------------------------------------------------------===//
  // Iteration Methods
  //===--------------------------------------------------------------------===//

  SubnetFragment next() override;

private:

  //===--------------------------------------------------------------------===//
  // Internal Methods
  //===--------------------------------------------------------------------===//

  /**
   * @brief Finds cut.
   * @param subnetBuilder Subnet for iteration.
   * @param root Index of root node.
   * @param cutSize Max cut size.
   */
  SubnetFragment getCut(const SubnetBuilder &subnetBuilder,
                        size_t root,
                        size_t cutSize);

  /**
   * @brief Gets a cone from the cut.
   * @param cut Indexes of cut's nodes.
   * @param root Index of root node.
   */
  SubnetFragment getFragment(std::unordered_set<size_t> &cut,
                             size_t root);


  SafePasser &start;
  size_t cutSize;
  size_t maxCones;
};

} // namespace eda::gate::optimizer
