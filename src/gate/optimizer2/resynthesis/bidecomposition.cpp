//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer2/resynthesis/bidecomposition.h"

#include <algorithm>

namespace eda::gate::optimizer2::resynthesis {

BiDecomposition::Link BiDecomposition::run(const KittyTT &func,
                                           const LinkList &inputs,
                                           SubnetBuilder &subnetBuilder,
                                           uint16_t maxArity) const {
  
  KittyTT care(func.num_vars());
  kitty::create_from_binary_string(care, std::string(func.num_bits(), '1'));
  TernaryBiClique initBiClique(func, care);

  return decompose(initBiClique, subnetBuilder, maxArity);
}

BiDecomposition::Link BiDecomposition::decompose(TernaryBiClique &initBiClique,
                                                 SubnetBuilder &subnetBuilder,
                                                 uint16_t maxArity) {

  if (initBiClique.getOnSet().size() == 1) {
    MinatoMorrealeAlg minatoMorrealeAlg;
    return minatoMorrealeAlg.synthFromISOP(initBiClique.getOnSet(),
        initBiClique.getInputs(), subnetBuilder, maxArity);
  }
  
  auto starBiCliques = initBiClique.getStarCoverage();

  auto [first, second] = findBaseCoverage(starBiCliques);
  expandBaseCoverage(starBiCliques, first, second);

  TernaryBiClique firstBiClique(initBiClique.getOffSet(),
                                std::move(first.offSet),
                                first.vars,
                                initBiClique.getInputs(),
                                initBiClique.getIndices());

  TernaryBiClique secondBiClique(std::move(initBiClique.getOffSet()),
                                 std::move(second.offSet),
                                 second.vars, 
                                 std::move(initBiClique.getInputs()),
                                 initBiClique.getIndices());

  Link lhs = decompose(firstBiClique, subnetBuilder, maxArity);
  Link rhs = decompose(secondBiClique, subnetBuilder, maxArity);

  return ~subnetBuilder.addCell(model::AND, lhs, rhs);
}

BiDecomposition::CoveragePair BiDecomposition::findBaseCoverage(
    CoverageList &stars) {

  auto first = stars.end() - 2;
  auto second = stars.end() - 1;

  auto intersection = __builtin_popcount(first->vars & second->vars);
  auto merge = __builtin_popcount(first->vars | second->vars);
 
  for (auto i = stars.begin(); i != (stars.end() - 2); ++i) {
    for (auto j = (i + 1); j != stars.end(); ++j) {
      auto newIntersection = __builtin_popcount(i->vars & j->vars);
      auto newMerge = __builtin_popcount(i->vars | j->vars);
      if (newIntersection < intersection ||
         (newIntersection == intersection && newMerge > merge)) {
        first = i;
        second = j;
        merge = newMerge;
        intersection = newIntersection;
      }
    }
  }

  auto result = std::pair(*first, *second);

  stars.erase(first);
  stars.erase(std::find(stars.begin(), stars.end(), result.second));

  return result;
}

void BiDecomposition::expandBaseCoverage(CoverageList &stars, Coverage &first,
                                         Coverage &second) {
  while (!stars.empty()) {
    bool shouldWiden = true;
    auto absorbed = stars.end();
    uint8_t difBase = 32;
    uint8_t difAbsorbed = 32;
    for (auto it = stars.begin(); it != stars.end(); ++it) {
      if (checkExpanding(difBase, difAbsorbed, first, *it)) {
        shouldWiden = true;
        absorbed = it;
      }
      if (checkExpanding(difBase, difAbsorbed, first, *it)) {
        shouldWiden = false;
        absorbed = it;
      }
    }
    if (shouldWiden) {
      first.vars = first.vars | absorbed->vars;
      first.offSet.push_back(*absorbed->offSet.begin());
    } else {
      second.vars = second.vars | absorbed->vars;
      second.offSet.push_back(*absorbed->offSet.begin());
    }
    stars.erase(absorbed); 
  }
}

bool BiDecomposition::checkExpanding(uint8_t &difBase, uint8_t &difAbsorbed,
                                     Coverage &first, Coverage &second) {
  uint8_t newMerge = __builtin_popcount(first.vars | second.vars);
  uint8_t newDifBase = newMerge - __builtin_popcount(first.vars);
  uint8_t newDifAbsorbed = newMerge - __builtin_popcount(second.vars);
  if (newDifBase < difBase ||
     (newDifBase == difBase && newDifAbsorbed < difAbsorbed)) {
    difBase = newDifBase;
    difAbsorbed = newDifAbsorbed;
    return true;
  }
  return false;
}

} // namespace eda::gate::optimizer2::resynthesis
