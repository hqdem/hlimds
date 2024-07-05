//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"
#include "gate/optimizer/synthesis/isop.h"
#include "gate/optimizer/synthesis/ternary_bi_clique.h"
#include "gate/optimizer/synthesizer.h"
#include "util/kitty_utils.h"

#include <utility>
#include <vector>

namespace eda::gate::optimizer::synthesis {

/**
 * \brief Implements method of synthesis, by means of a heuristic method for
 * bi-decomposition of Boolean functions.
 *
 * The algorithm based on the article "Synthesis of combinational circuits by
 * means of bi-decomposition of Boolean functions" by Yuri V. Pottosin (2022).
*/
class BiDecSynthesizer final : public TruthTableSynthesizer {
public:

  using Coverage      = TernaryBiClique::Coverage;
  using CoverageList  = std::vector<Coverage>;
  using CoveragePair  = std::pair<Coverage, Coverage>;
  using Link          = model::Subnet::Link;
  using LinkList      = model::Subnet::LinkList;
  using Subnet        = model::Subnet;
  using SubnetBuilder = model::SubnetBuilder;
  using SubnetObject  = model::SubnetObject;

  using Synthesizer::synthesize;

  /// Synthesizes a subnet.
  SubnetObject synthesize(const TruthTable &func, const TruthTable &care,
                          uint16_t maxArity = -1) const override {
    if (bool value; utils::isConst(func, value)) {
      return SubnetObject{SubnetBuilder::makeConst(func.num_vars(), value)};
    }
    if (bool value; care.num_vars() && utils::isConst((func & care), value)) {
      return SubnetObject{SubnetBuilder::makeConst(func.num_vars(), value)};
    }
    return run(func, care, maxArity);
  }

private:

  static SubnetObject run(const TruthTable &func, const TruthTable &care,
                          uint16_t maxArity);

  static Link decompose(TernaryBiClique &initBiClique,
                        SubnetBuilder &subnetBuilder, uint16_t maxArity = -1);

  static CoveragePair findBaseCoverage(CoverageList &stars);

  static void expandBaseCoverage(CoverageList &stars, Coverage &first,
                                 Coverage &second);

  static bool checkExpanding(uint8_t &difBase, uint8_t &difAbsorbed,
                             Coverage &first, Coverage &second);
  
};

} // namespace eda::gate::optimizer::synthesis
