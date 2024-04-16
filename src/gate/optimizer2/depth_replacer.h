//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer2/replacer.h"

namespace eda::gate::optimizer2 {

/**
 * @brief Replacer for Depth resynthesis optimization.
 */
class DepthReplacer final : public ReplacerBase {
public:

  /// Alias for SubnetID.
  using SubnetID = ReplacerBase::SubnetID;

  //===--------------------------------------------------------------------===//
  // Constructors/Destructors
  //===--------------------------------------------------------------------===//

  /**
   * @brief Depth optimizer constructor for subnet builder.
   * @param subnetBuilder Subnet for replacing.
   */
  DepthReplacer(SubnetBuilder &subnetBuilder) : ReplacerBase(subnetBuilder) {}

  virtual void replace(SubnetFragment lhs, SubnetID rhs);

  virtual void finalize() {}
};

} // namespace eda::gate::optimizer2

