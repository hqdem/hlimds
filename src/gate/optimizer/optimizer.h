//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"
#include "gate/optimizer/replacer.h"
#include "gate/optimizer/resynthesizer.h"
#include "gate/optimizer/subnet_iterator.h"

namespace eda::gate::optimizer {

/**
 * @brief Represents template facade for optimization subsystem.
 */
class OptimizerBase {
public:

  /// @cond ALIASES
  using Subnet        = eda::gate::model::Subnet;
  using SubnetBuilder = eda::gate::model::SubnetBuilder;
  using SubnetID      = eda::gate::model::SubnetID;
  /// @endcond

  /**
   * @brief Optimizes the subnet.
   */
  void optimize() {
    for (auto old{iterator->next()}; old.isValid(); old = iterator->next()) {
      SubnetID update = resynthesizer->resynthesize(old);
      replacer->replace(old, update);
    }
    replacer->finalize();
  }

  virtual ~OptimizerBase() = default;

protected:
  
  OptimizerBase() {}

  /// Iterator over the subnet.
  SubnetIteratorBase *iterator;
  /// Resynthesizer.
  ResynthesizerBase *resynthesizer;
  /// Replacer.
  ReplacerBase *replacer;
};

/* TODO: Fix the stub.
class PowerRewritter final : public OptimizerBase {
public:

  PowerReWritter(SubnetBuilder &subnetBuilder) {
    iterator = new PowerSubnetIterator(subnetBuilder);
    resynthesizer = new PowerResynthesizer(subnetBuilder);
    replacer = new PowerReplacer(subnetBuilder);
  }

  ~PowerReWritter() {
    delete iterator;
    delete resynthesizer;
    delete replacer;
  }

};
*/

enum OptimizationCriterion {
  NoOpt,
  Area,
  Delay,
  Power,
};

} // namespace eda::gate::optimizer
