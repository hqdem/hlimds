//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnetview.h"
#include "gate/model/utils/subnet_truth_table.h"
#include "gate/optimizer/mffc.h"
#include "gate/optimizer/reconvergence.h"
#include "gate/optimizer/safe_passer.h"
#include "gate/optimizer/subnet_transformer.h"
#include "util/truth_table.h"

#include <string>

namespace eda::gate::optimizer {

/// @brief Implements a resubstitution algorithm of optimization.
class Resubstitutor : public SubnetInPlaceTransformer {
public:

  using Subnet        = eda::gate::model::Subnet;
  using SubnetBuilder = eda::gate::model::SubnetBuilder;

  /**
   * @brief Constructs a resubstitutor.
   *
   * @param cutSize Maximum number of elements in the cut.
   * @param maxLevels Maximum levels from a pivot to roots for care evaluation.
   * @param zero Enables zero replacements.
   * @param saveDepth Depth preserving flag.
   */
  Resubstitutor(const std::string &name,
                unsigned cutSize,
                unsigned maxLevels,
                bool zero,
                bool saveDepth) :
      SubnetInPlaceTransformer(name),
      cutSize(cutSize),
      maxLevels(maxLevels),
      zero(zero),
      saveDepth(saveDepth) {}

  /// @brief Constructs a resubstitutor with default parameters.
  Resubstitutor(const std::string &name) :
      Resubstitutor(name, 8, 3, false, false) {}

  /// @brief Tranforms the subnet by applying a resubstitution algorithm.
  void transform(const SubnetBuilderPtr &builder) const override;

private:
  const unsigned cutSize;
  const unsigned maxLevels;
  const bool zero;
  const bool saveDepth;
};

} // namespace eda::gate::optimizer
