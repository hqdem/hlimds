//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/replacer.h"
#include "gate/optimizer/safe_passer.h"

namespace eda::gate::optimizer {

/**
 * @brief Replacer for Depth resynthesis optimization.
 */
class DepthReplacer final : public ReplacerBase {
public:

  using SafePasser = eda::gate::optimizer::SafePasser;
  using SubnetID = ReplacerBase::SubnetID;

  //===--------------------------------------------------------------------===//
  // Constructors/Destructors
  //===--------------------------------------------------------------------===//

  /**
   * @brief Depth optimizer constructor for subnet builder.
   * @param subnetBuilder Subnet for replacing.
   */
  DepthReplacer(SubnetBuilder &subnetBuilder, SafePasser &iter) :
      ReplacerBase(subnetBuilder), iter(iter) {}

  virtual void replace(SubnetFragment lhs, SubnetID rhs);

  virtual void finalize() {}

private:
  SafePasser &iter;
};

} // namespace eda::gate::optimizer

