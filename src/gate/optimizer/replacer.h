//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"
#include "gate/optimizer/subnet_iterator.h"

namespace eda::gate::optimizer {

/**
 * @brief A common interface for replacing in the subnet.
 */
class ReplacerBase {
public:

  /// @cond ALIASES
  using Subnet         = eda::gate::model::Subnet;
  using SubnetBuilder  = eda::gate::model::SubnetBuilder;
  using SubnetFragment = SubnetIteratorBase::SubnetFragment;
  using SubnetID       = eda::gate::model::SubnetID;
  /// @endcond

  /**
   * @brief Constructor.
   * @param subnetBuilder The subnet for replacing.
   */
  ReplacerBase(SubnetBuilder &subnetBuilder) : subnetBuilder(subnetBuilder) {}

  /**
   * @brief Replaces lhs by rhs in the subnet.
   * @param lhs Fragment from subnet.
   * @param rhs The subnet for fragment replacing.
   */ 
  virtual void replace(SubnetFragment lhs, SubnetID rhs) = 0;

  /**
   * @brief Finalize replacements in the subnet.
   */ 
  virtual void finalize() = 0;

  virtual ~ReplacerBase() = default;

protected:

  /// The subnet for replacing.
  SubnetBuilder &subnetBuilder;
};

} // namespace eda::gate::optimizer
