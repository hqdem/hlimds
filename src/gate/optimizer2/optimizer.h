//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"
#include "gate/optimizer2/replacer.h"
#include "gate/optimizer2/resynthesizer.h"
#include "gate/optimizer2/subnet_iterator.h"

#include <tuple>
#include <utility>

namespace eda::gate::optimizer2 {

/**
 * @brief Represents facade for optimization subsystem.
 */
class OptimizerBase {
public:
  
  /// @cond ALIASES
  using LinkList      = eda::gate::model::Subnet::LinkList;
  using Subnet        = eda::gate::model::Subnet;
  using SubnetBuilder = eda::gate::model::SubnetBuilder;
  using SubnetID      = eda::gate::model::SubnetID;
  /// @endcond

  /**
   * @brief Optimizes the subnet.
   */
  void optimize() {
    for (auto old{iterator.next()}; old.isValid(); old = iterator.next()) {
      SubnetID update = resynthesizer.resynthesize(old.subnetID);
      replacer.replace(old, update);
    }
    replacer.finalize();
  }

  virtual ~OptimizerBase() {};

protected:

  OptimizerBase(SubnetBuilder &subnetBuilder, SubnetIteratorBase &iterator,
                ResynthesizerBase &resynthesizer, ReplacerBase &replacer) :
      subnetBuilder(subnetBuilder), iterator(iterator),
      resynthesizer(resynthesizer), replacer(replacer) {};
  
  /// The subnet for optimization.
  SubnetBuilder &subnetBuilder;
  /// Iterator over the subnet.
  SubnetIteratorBase &iterator;
  /// Resynthesizer.
  ResynthesizerBase &resynthesizer;
  /// Replacer.
  ReplacerBase &replacer;
};

} // namespace eda::gate::optimizer2
