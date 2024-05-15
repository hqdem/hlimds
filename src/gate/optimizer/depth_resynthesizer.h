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

/**
 * @brief Resynthesizer for depth optimization.
 */
class DepthResynthesizer final : public ResynthesizerBase {
public:

  using ResynthesizerBase::ResynthesizerBase;

  // Alias for SubnetID.
  using SubnetID = eda::gate::model::SubnetID;

  SubnetID resynthesize(SubnetID subnetID) const override {
    synthesis::MMSynthesizer minato;
    const auto &subnet = model::Subnet::get(subnetID);
    return minato.synthesize(model::evaluateSingleOut(subnet), 2);
  }
};

} // namespace eda::gate::optimizer

