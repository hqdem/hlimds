//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"
#include "gate/optimizer/synthesis/isop.h"
#include "gate/optimizer/synthesis/ternary_bi_clique.h"
#include "gate/optimizer/synthesizer.h"

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
class BiDecSynthesizer final : public Synthesizer<kitty::dynamic_truth_table> {
public:

  using Coverage      = TernaryBiClique::Coverage;
  using CoverageList  = std::vector<Coverage>;
  using CoveragePair  = std::pair<Coverage, Coverage>;
  using KittyTT       = kitty::dynamic_truth_table;
  using Link          = model::Subnet::Link;
  using LinkList      = model::Subnet::LinkList;
  using Subnet        = model::Subnet;
  using SubnetBuilder = model::SubnetBuilder;
  using SubnetID      = model::SubnetID;

  /// Synthesizes the Subnet.
  SubnetID synthesize(const KittyTT &func,
                      uint16_t maxArity = -1) const override {
    bool one{kitty::is_const0(~func)};
    bool zero{ kitty::is_const0(func)};
    if (one || zero) {
      return synthConstFunc(func.num_vars(), one);
    }
    return run(func, maxArity);
  }

private:

  static SubnetID run(const KittyTT &func, uint16_t maxArity);

  static Link decompose(TernaryBiClique &initBiClique,
                        SubnetBuilder &subnetBuilder, uint16_t maxArity = -1);

  static CoveragePair findBaseCoverage(CoverageList &stars);

  static void expandBaseCoverage(CoverageList &stars, Coverage &first,
                                 Coverage &second);

  static bool checkExpanding(uint8_t &difBase, uint8_t &difAbsorbed,
                             Coverage &first, Coverage &second);
  
};

} // namespace eda::gate::optimizer::synthesis
