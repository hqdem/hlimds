//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"
#include "gate/model/utils/subnet_truth_table.h"
#include "gate/optimizer/synthesizer.h"

#include <kitty/dynamic_truth_table.hpp>

#include <cassert>
#include <cstdint>

namespace eda::gate::optimizer {

/**
 * @brief Common interface for subnet-to-subnet resynthesizers.
 */
class ResynthesizerBase {
public:
  using TruthTable = kitty::dynamic_truth_table;

  ResynthesizerBase() = default;
  virtual ~ResynthesizerBase() = default;

  /**
   * @brief Resynthesizes the subnet w/ the care specification.
   * @return The identifier of the newly constructed subnet or OBJ_NULL_ID.
   */
  virtual model::SubnetID resynthesize(
      const model::SubnetID subnetID,
      const TruthTable &care,
      const uint16_t maxArity = -1) const = 0;

  /**
   * @brief Resynthesizes the subnet.
   * @return The identifier of the newly constructed subnet or OBJ_NULL_ID.
   */
  model::SubnetID resynthesize(
      const model::SubnetID subnetID,
      const uint16_t maxArity = -1) const {
    return resynthesize(subnetID, TruthTable{}, maxArity);
  }
};

/**
 * @brief Constructs the subnet IR.
 */
template<typename IR>
IR construct(const model::Subnet &subnet);

/**
 * @brief Subnet-to-subnet resynthesizer based on the IR representation.
 */
template<typename IR>
class Resynthesizer final : public ResynthesizerBase {
public:
  Resynthesizer(const Synthesizer<IR> &synthesizer):
    synthesizer(synthesizer) {}

  using ResynthesizerBase::resynthesize;

  model::SubnetID resynthesize(
      const model::SubnetID subnetID,
      const TruthTable &care,
      const uint16_t maxArity = -1) const override {
    assert(subnetID != model::OBJ_NULL_ID);
    const auto ir = construct<IR>(model::Subnet::get(subnetID));
    return synthesizer.synthesize(ir, care, maxArity);
  }

private:
  const Synthesizer<IR> &synthesizer;
};

template<>
inline kitty::dynamic_truth_table construct<kitty::dynamic_truth_table>(
    const model::Subnet &subnet) {
  return model::evaluateSingleOut(subnet);
}

} // namespace eda::gate::optimizer
