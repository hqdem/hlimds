//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"
#include "gate/optimizer/subnet_window.h"
#include "gate/optimizer/synthesizer.h"

#include <kitty/kitty.hpp>

#include <cassert>
#include <cstdint>

namespace eda::gate::optimizer {

/**
 * @brief Common interface for subnet-to-subnet resynthesizers.
 */
class ResynthesizerBase {
public:
  ResynthesizerBase() = default;
  virtual ~ResynthesizerBase() = default;

  /**
   * @brief Resynthesizes the subnet window.
   * @return The identifier of the newly constructed subnet or OBJ_NULL_ID.
   */
  virtual model::SubnetID resynthesize(const SubnetWindow &window,
                                       const uint16_t maxArity = -1) const = 0;
};

/**
 * @brief Constructs the subnet IR.
 */
template<typename IR>
IR construct(const SubnetWindow &window);

/**
 * @brief Subnet-to-subnet resynthesizer based on the IR representation.
 */
template<typename IR>
class Resynthesizer final : public ResynthesizerBase {
public:
  Resynthesizer(const Synthesizer<IR> &synthesizer):
      synthesizer(synthesizer) {}

  model::SubnetID resynthesize(const SubnetWindow &window,
                               const uint16_t maxArity = -1) const override {
    const auto ir = construct<IR>(window);
    return synthesizer.synthesize(ir, window.getCare(), maxArity);
  }

private:
  const Synthesizer<IR> &synthesizer;
};

template<>
inline kitty::dynamic_truth_table construct<kitty::dynamic_truth_table>(
    const SubnetWindow &window) {
  return window.evaluateTruthTable();
}

} // namespace eda::gate::optimizer
