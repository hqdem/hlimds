//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"
#include "gate/optimizer2/resynthesis/ternary_bi_clique.h"
#include "gate/optimizer2/synthesizer.h"

#include <vector>

namespace eda::gate::optimizer2::resynthesis {

/**
 * \brief Implements method of resynthesis, by means of a heuristic method for
 * bi-decomposition of Boolean functions.
 *
 * The algorithm based on the article "Synthesis of combinational circuits by
 * means of bi-decomposition of Boolean functions" by Yuri V. Pottosin (2022).
*/
class BiDecompositor : public Synthesizer {
public:

  using CoverageElement = TernaryBiClique::CoverageElement;
  using Subnet          = eda::gate::model::Subnet;
  using SubnetID        = eda::gate::model::SubnetID;
  using Link            = Subnet::Link;
  using LinkList        = Subnet::LinkList;
  using SubnetBuilder   = eda::gate::model::SubnetBuilder;
  using TruthTable      = Synthesizer::TruthTable;

  BiDecompositor() { };

  ~BiDecompositor() { };

  /** Launchs synthesizing the Subnet.
   * @param func To define a Boolean function.
   * @return SunetID.
  */
  SubnetID synthesize(const TruthTable &func) override;

private:

  static Link getBiDecomposition(TernaryBiClique &initBiClique,
                                 const std::vector<size_t> &inputs,
                                 SubnetBuilder &subnetBuilder);

  static std::pair<CoverageElement, CoverageElement> 
      findBaseCoverage(std::vector<CoverageElement> &stars);

  static void expandBaseCoverage(std::vector<CoverageElement> &stars,
                                 CoverageElement &first,
                                 CoverageElement &second);

  static bool checkExpanding(uint8_t &difBase, uint8_t &difAbsorbed,
                             CoverageElement &first, CoverageElement &second);
  
  static Link makeNetForDNF(const TernaryVector &vector,
                            const std::vector<size_t> &inputs,
                            SubnetBuilder &subnetBuilder);
};

} // namespace eda::gate::optimizer2::resynthesis
