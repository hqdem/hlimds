//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"
#include "util/truth_table.h"

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
      const utils::TruthTable &care,
      const uint16_t maxArity = -1) const = 0;

  /**
   * @brief Synthesizes a subnet for the given IR.
   * @return The identifier of the newly constructed subnet or OBJ_NULL_ID.
   */
  model::SubnetObject synthesize(
      const IR &ir,
      const uint16_t maxArity = -1) const {
    return synthesize(ir, utils::TruthTable{}, maxArity);
  }  
};

/// Truth-table-based synthesizer.
using TruthTableSynthesizer = Synthesizer<utils::TruthTable>;

} // namespace eda::gate::optimizer
