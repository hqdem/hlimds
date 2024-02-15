//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"
#include "gate/model2/utils/subnet_truth_table.h"
#include "gate/optimizer2/synthesizer.h"

#include "kitty/dynamic_truth_table.hpp"

#include <cassert>

namespace eda::gate::optimizer2 {

/// Resynthesizer interface: Subnet -> Subnet.
class ResynthesizerBase {
public:
  using SubnetID = eda::gate::model::SubnetID;

  /// Resynthesizes the given subnet.
  /// Returns the identifier of the newly constructed subnet.
  virtual SubnetID resynthesize(SubnetID subnetID) = 0;
};

template<typename IR>
IR construct(const eda::gate::model::Subnet &subnet);

template<typename IR>
class Resynthesizer final : public ResynthesizerBase {
public:
  Resynthesizer(const Synthesizer<IR> &&synthesizer):
    synthesizer(synthesizer) {}

  SubnetID resynthesize(SubnetID subnetID) override {
    assert(subnetID != eda::gate::model::OBJ_NULL_ID);
    const auto ir = construct<IR>(eda::gate::model::Subnet::get(subnetID));
    return synthesizer.synthesize(ir);
  }

private:
  const Synthesizer<IR> &synthesizer;
};

template<>
kitty::dynamic_truth_table construct<kitty::dynamic_truth_table>(
    const eda::gate::model::Subnet &subnet) {
  return eda::gate::model::evaluateSingleOut(subnet);
}

} // namespace eda::gate::optimizer2
