//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/bgnet.h"
#include "gate/optimizer2/resynthesis/ternary_bi_clique.h"

namespace eda::gate::optimizer2::resynthesis {

/**
 * \brief Implements method of resynthesis, by means of a heuristic method for
 * bi-decomposition of Boolean functions.
 *
 * The algorithm based on the article "Synthesis of combinational circuits by
 * means of bi-decomposition of Boolean functions" by Yuri V. Pottosin (2022).
*/
class BiDecompositor final {
public:

  using BoundGNet       = eda::gate::optimizer::BoundGNet;
  using CoverageElement = TernaryBiClique::CoverageElement;
  using GNet            = eda::gate::model::GNet;
  using GateId          = GNet::GateId;
  using GateIdList      = GNet::GateIdList;
  using KittyTT         = TernaryBiClique::KittyTT;

  BiDecompositor() { };

  ~BiDecompositor() { };

  /** Runs bi-decomposition.
   * @param func To define a Boolean function.
   * @param care To define don't care rows in func.
   * @return Pointer of GNet and lists of inputs and outputs - BoundGNet.
  */
  static BoundGNet run(const KittyTT &func, const KittyTT &care);

private:

  static GateId getBiDecomposition(TernaryBiClique &initBiClique,
                                   const GateIdList &inputs, GNet &net);

  static std::pair<CoverageElement, CoverageElement> 
      findBaseCoverage(std::vector<CoverageElement> &stars);

  static void expandBaseCoverage(std::vector<CoverageElement> &stars,
                                 CoverageElement &first,
                                 CoverageElement &second);

  static bool checkExpanding(uint8_t &difBase, uint8_t &difAbsorbed,
                             CoverageElement &first, CoverageElement &second);
  
  static GateId makeNetForDNF(const TernaryVector &vector,
                              const GateIdList &inputs, GNet &net);
};

} // namespace eda::gate::optimizer2::resynthesis
