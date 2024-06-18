//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/synthesis/bidecomposition.h"
#include "gate/optimizer/synthesis/isop.h"

#include <algorithm>

namespace eda::gate::optimizer::synthesis {

/// @cond ALIASES
using CoveragePair = BiDecSynthesizer::CoveragePair;
using Link         = BiDecSynthesizer::Link;
using SubnetID     = BiDecSynthesizer::SubnetID;
/// @endcond

SubnetID BiDecSynthesizer::run(const TruthTable &func, const TruthTable &care,
                               uint16_t maxArity) {
  SubnetBuilder subnetBuilder;
  LinkList inputs = subnetBuilder.addInputs(func.num_vars());
  TernaryBiClique initBiClique(func,
      care.num_vars() ? care : utils::generateConstTT(func.num_vars()));
  subnetBuilder.addOutput(decompose(initBiClique, subnetBuilder, maxArity));
  return subnetBuilder.make();
}

Link BiDecSynthesizer::decompose(TernaryBiClique &initBiClique,
                                 SubnetBuilder &subnetBuilder,
                                 uint16_t maxArity) {
  if (initBiClique.getOnSet().size() == 1) {
    return synthFromSOP(initBiClique.getOnSet(),
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

CoveragePair BiDecSynthesizer::findBaseCoverage(CoverageList &stars) {

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

void BiDecSynthesizer::expandBaseCoverage(CoverageList &stars, Coverage &first,
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

bool BiDecSynthesizer::checkExpanding(uint8_t &difBase, uint8_t &difAbsorbed,
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

} // namespace eda::gate::optimizer::synthesis
