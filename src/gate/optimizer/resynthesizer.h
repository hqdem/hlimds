//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"
#include "gate/model/utils/subnet_truth_table.h"
#include "gate/optimizer/synthesizer.h"

#include "kitty/dynamic_truth_table.hpp"

#include <cassert>

namespace eda::gate::optimizer {

/// Resynthesizer interface: Subnet -> Subnet.
class ResynthesizerBase {
public:
  using TruthTable = kitty::dynamic_truth_table;
  using SubnetID   = eda::gate::model::SubnetID;

  /// Resynthesizes the given subnet with don't care.
  /// Returns the identifier of the newly constructed subnet or OBJ_NULL_ID.
  virtual SubnetID resynthesize(SubnetID subnetID, const TruthTable &care,
                                uint16_t maxArity = -1) const = 0;

  /// Resynthesizes the given subnet.
  /// Returns the identifier of the newly constructed subnet or OBJ_NULL_ID.
  SubnetID resynthesize(SubnetID subnetID, uint16_t maxArity = -1) const {
    return resynthesize(subnetID, TruthTable(), maxArity);
  };

  virtual ~ResynthesizerBase() = default;
};

template<typename IR>
IR construct(const eda::gate::model::Subnet &subnet);

template<typename IR>
class Resynthesizer final : public ResynthesizerBase {
public:
  Resynthesizer(const Synthesizer<IR> &synthesizer):
    synthesizer(synthesizer) {}

  using ResynthesizerBase::resynthesize;

  SubnetID resynthesize(SubnetID subnetID, const TruthTable &care,
                        uint16_t maxArity = -1) const override {
    assert(subnetID != eda::gate::model::OBJ_NULL_ID);
    const auto ir = construct<IR>(eda::gate::model::Subnet::get(subnetID));
    return synthesizer.synthesize(ir, care, maxArity);
  }

private:
  const Synthesizer<IR> &synthesizer;
};

template<>
inline kitty::dynamic_truth_table construct<kitty::dynamic_truth_table>(
    const eda::gate::model::Subnet &subnet) {
  return eda::gate::model::evaluateSingleOut(subnet);
}

} // namespace eda::gate::optimizer
