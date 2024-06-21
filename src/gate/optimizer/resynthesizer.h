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
#include "gate/optimizer/cone_builder.h"
#include "gate/optimizer/cut_extractor.h"
#include "gate/optimizer/synthesizer.h"

#include <kitty/dynamic_truth_table.hpp>

#include <cassert>
#include <cstdint>
#include <unordered_map> // FIXME:

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
      // FIXME: const model::SubnetID subnetID,
      const model::SubnetBuilder &builder,
      const CutExtractor::Cut &cut,
      std::unordered_map<size_t, size_t> &mapping, // FIXME:
      const TruthTable &care,
      const uint16_t maxArity = -1) const = 0;

  /**
   * @brief Resynthesizes the subnet.
   * @return The identifier of the newly constructed subnet or OBJ_NULL_ID.
   */
  model::SubnetID resynthesize(
      // FIXME: const model::SubnetID subnetID,
      const model::SubnetBuilder &builder,
      const CutExtractor::Cut &cut,
      std::unordered_map<size_t, size_t> &mapping, // FIXME:
      const uint16_t maxArity = -1) const {
    return resynthesize(builder, cut, mapping, TruthTable{}, maxArity);
  }
};

/**
 * @brief Constructs the subnet IR.
 */
template<typename IR>
IR construct(// FIXME: const model::Subnet &subnet
    const model::SubnetBuilder &builder,
    const CutExtractor::Cut &cut,
    std::unordered_map<size_t, size_t> &mapping); // FIXME:

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
      // FIXME: const model::SubnetID subnetID,
      const model::SubnetBuilder &builder,
      const CutExtractor::Cut &cut,
      std::unordered_map<size_t, size_t> &mapping,
      const TruthTable &care,
      const uint16_t maxArity = -1) const override {
    // FIXME: assert(subnetID != model::OBJ_NULL_ID);
    const auto ir = construct<IR>(builder, cut, mapping /*FIXME: model::Subnet::get(subnetID)*/);
    return synthesizer.synthesize(ir, care, maxArity);
  }

private:
  const Synthesizer<IR> &synthesizer;
};

template<>
inline kitty::dynamic_truth_table construct<kitty::dynamic_truth_table>(
    // FIXME: const model::Subnet &subnet*/
    const model::SubnetBuilder &builder,
    const CutExtractor::Cut &cut,
    std::unordered_map<size_t, size_t> &mapping) { // FIXME:
  // FIXME: Evaluate the truth table directly from the cut.
  const ConeBuilder coneBuilder(&builder);
  const auto &cone = coneBuilder.getCone(cut);
  const auto &subnet = model::Subnet::get(cone.subnetID);

  // FIXME: Remove.
  for (const auto &[r, l]: cone.inOutToOrig) {
    mapping.emplace(r, l);
  }

  return model::evaluateSingleOut(subnet);
}

} // namespace eda::gate::optimizer
