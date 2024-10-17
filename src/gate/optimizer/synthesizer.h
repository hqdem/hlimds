//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/function/bdd.h"
#include "gate/function/truth_table.h"
#include "gate/model/subnet.h"

#include <cassert>
#include <cstdint>

namespace eda::gate::optimizer {

/**
 * @brief Common interface for synthesizers.
 */
template<typename IR>
class Synthesizer {
public:
  using FunctionIR = IR;
  
  Synthesizer() = default;
  virtual ~Synthesizer() = default;

  /**
   * @brief Synthesizes a subnet for the given IR and care specification.
   * @return The identifier of the newly constructed subnet or OBJ_NULL_ID.
   */
  virtual model::SubnetObject synthesize(
      const IR &ir,
      const model::TruthTable &care,
      const uint16_t maxArity = -1) const = 0;

  /**
   * @brief Synthesizes a subnet for the given IR.
   * @return The identifier of the newly constructed subnet or OBJ_NULL_ID.
   */
  model::SubnetObject synthesize(
      const IR &ir,
      const uint16_t maxArity = -1) const {
    return synthesize(ir, model::TruthTable{}, maxArity);
  }
};

/// BDD-based synthesizer.
using BddSynthesizer = Synthesizer<model::Bdd>;

/// TT-based synthesizer.
using TruthTableSynthesizer = Synthesizer<model::TruthTable>;

} // namespace eda::gate::optimizer
