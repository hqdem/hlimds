//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/utils/subnet_truth_table.h"
#include "gate/optimizer/mffc.h"
#include "gate/optimizer/reconvergence_cut.h"
#include "gate/optimizer/safe_passer.h"
#include "gate/optimizer/subnet_iterator.h"
#include "gate/optimizer/subnet_transformer.h"

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
   * @param careSize Maximum number of elements in the cut for care evaluation.
   */
  Resubstitutor(const std::string &name, unsigned cutSize, unsigned careSize) :
      SubnetInPlaceTransformer(name), cutSize(cutSize), careSize(careSize) {}

  /// @brief Constructs a resubstitutor with default parameters.
  Resubstitutor(const std::string &name) : Resubstitutor(name, 8, 16) {}

  /// @brief Tranforms the subnet by applying a resubstitution algorithm.
  void transform(SubnetBuilder &builder) const override;

private:
  const unsigned cutSize;
  const unsigned careSize;
};

} // namespace eda::gate::optimizer
