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
  model::SubnetID resynthesize(
      model::SubnetID subnetID,
      const TruthTable &care,
      const uint16_t maxArity = -1) const override;
};

} // namespace eda::gate::optimizer

