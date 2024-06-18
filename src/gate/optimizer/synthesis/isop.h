//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"
#include "gate/optimizer/synthesis/algebraic_factor.h"
#include "gate/optimizer/synthesizer.h"
#include "util/kitty_utils.h"

#include "kitty/kitty.hpp"

#include <utility>
#include <vector>

namespace eda::gate::optimizer::synthesis {

/// @cond ALIASES
using Cube          = kitty::cube;
using Link          = eda::gate::model::Subnet::Link;
using LinkList      = eda::gate::model::Subnet::LinkList;
using SOP           = std::vector<Cube>;
using SubnetBuilder = eda::gate::model::SubnetBuilder;
using SubnetID      = model::SubnetID;
/// @endcond

/// Synthesizes the subnet from SOP of the boolean function.
Link synthFromSOP(const SOP &sop, const LinkList &inputs,
                  SubnetBuilder &subnetBuilder, uint16_t maxArity = -1);

/// Synthesizes the subnet from cube.
Link synthFromCube(Cube cube, const LinkList &inputs,
                   SubnetBuilder &subnetBuilder, int16_t maxArity = -1);

/**
 * \brief Implements method of synthesis, by Minato-Morreale algorithm.
*/
class MMSynthesizer final : public TruthTableSynthesizer {
public:

  using Synthesizer::synthesize;

  /// Synthesizes the Subnet.
  SubnetID synthesize(const TruthTable &func,
                      const TruthTable &care,
                      uint16_t maxArity = -1) const override;

  using Synthesizer::synthesizeWithFactoring;

  /// Synthesizes the Subnet.
  SubnetID synthesizeWithFactoring(const TruthTable &func,
                                   const TruthTable &care,
                                   uint16_t maxArity = -1) const override;

private:

  std::pair<TruthTable, bool> handleCare(const TruthTable &func,
                                         const TruthTable &care) const;

  /// For the synthesis of Boolean functions with algebraic factoring.
  AlgebraicFactor factor;
};

} // namespace eda::gate::optimizer::synthesis
