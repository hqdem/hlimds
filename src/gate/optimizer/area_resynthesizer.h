//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/resynthesizer.h"

namespace eda::gate::optimizer {

/// Implements a resynthesizer for the area optimization.
class AreaResynthesizer : public ResynthesizerBase {
public:
  using Subnet        = eda::gate::model::Subnet;
  using SubnetBuilder = eda::gate::model::SubnetBuilder;

  AreaResynthesizer(SubnetBuilder &subnetBuilder, size_t arity = 2) :
    subnetBuilder(subnetBuilder), maxArity(arity) {}

  /// Resynthesizes the given subnet with care computation.
  /// Returns the identifier of the newly constructed subnet.
  SubnetID resynthesize(const SubnetFragment &sf) const override;
  SubnetID resynthesize(SubnetID subnetId) const override;

private:
  SubnetBuilder &subnetBuilder;
  size_t maxArity;
};

} // namespace eda::gate::optimizer

