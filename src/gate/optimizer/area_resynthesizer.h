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

  SubnetID resynthesize(SubnetID subnetId, const TruthTable &care,
                        uint16_t maxArity = -1) const override;

};

} // namespace eda::gate::optimizer

