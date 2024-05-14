//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer2/area_optimizer.h"
#include "gate/optimizer2/area_replacer.h"
#include "gate/optimizer2/area_resynthesizer.h"
#include "gate/optimizer2/area_subnet_iterator.h"
#include "gate/optimizer2/optimizer.h"
#include "gate/optimizer2/safe_passer.h"

namespace eda::gate::optimizer2 {

/**
 * @brief Implements the area optimization subsystem.
 */
class AreaOptimizer : public OptimizerBase {
public:
  using SafePasser = eda::gate::optimizer2::SafePasser;
  /**
   * @brief AreaOptimizer construstor.
   * @param arity Max arity of gates in resynthesized subnets.
   * @param cutSize The size of constructed cuts.
   */
  AreaOptimizer(SubnetBuilder &builder, size_t arity,
                size_t cutSize = 8, double delta = 0.0)
      : iter(builder.begin()) {
    iterator = new AreaSubnetIterator(builder, iter, cutSize);
    resynthesizer = new AreaResynthesizer(builder, arity);
    replacer = new AreaReplacer(builder, iter, delta);
  }

  ~AreaOptimizer() {
    delete iterator;
    delete resynthesizer;
    delete replacer;
  }

private:
  SafePasser iter;
};

} // namespace eda::gate::optimizer2
