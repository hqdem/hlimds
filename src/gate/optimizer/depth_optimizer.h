//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/depth_optimizer.h"
#include "gate/optimizer/depth_replacer.h"
#include "gate/optimizer/depth_resynthesizer.h"
#include "gate/optimizer/depth_subnet_iterator.h"
#include "gate/optimizer/optimizer.h"
#include "gate/optimizer/safe_passer.h"

namespace eda::gate::optimizer {

class DepthOptimizer final : public OptimizerBase {
public:

  using SafePasser = eda::gate::optimizer::SafePasser;
  using Subnet = eda::gate::model::Subnet;
  using SubnetBuilder = eda::gate::model::SubnetBuilder;

  //===--------------------------------------------------------------------===//
  // Constructors/Destructors
  //===--------------------------------------------------------------------===//

  /**
   * @brief DepthOptimizer construstor.
   * @param cutSize Max cut size.
   * @param maxCones Max number of cone constructions.
   */
  DepthOptimizer(SubnetBuilder &subnetBuilder,
                 size_t cutSize,
                 size_t maxCones = -1)
      : iter(subnetBuilder.begin()) {
    iterator = new DepthSubnetIterator(subnetBuilder, iter, cutSize, maxCones);
    resynthesizer = new DepthResynthesizer();
    replacer = new DepthReplacer(subnetBuilder, iter);
  }

  ~DepthOptimizer() {
    delete iterator;
    delete resynthesizer;
    delete replacer;
  }

private:
  SafePasser iter;
};

} // namespace eda::gate::optimizer
