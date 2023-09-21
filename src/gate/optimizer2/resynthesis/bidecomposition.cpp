//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer2/resynthesis/bidecomposition.h"
#include "util/assert.h"

#include <algorithm>
#include <cmath>
#include <memory>

namespace eda::gate::optimizer2::resynthesis {

using CoverageElement = BiDecompositor::CoverageElement;
using Link            = BiDecompositor::Link;
using SubnetID        = BiDecompositor::SubnetID;
using TruthTable      = BiDecompositor::TruthTable;

SubnetID BiDecompositor::synthesize(const TruthTable &func) {
  SubnetBuilder subnetBuilder;

  std::vector<size_t> inputs;
  for (size_t i = 0; i < func.num_vars(); ++i) {
    inputs.push_back(subnetBuilder.addCell(model::IN, SubnetBuilder::INPUT));
  }
  
  TruthTable care(func.num_vars());
  kitty::create_from_binary_string(care, std::string(func.num_bits(), '1'));
  TernaryBiClique initBiClique(func, care);

  Link output = getBiDecomposition(initBiClique, inputs, subnetBuilder);

  subnetBuilder.addCell(model::OUT, output, SubnetBuilder::OUTPUT);
  
  return subnetBuilder.make();
}

Link BiDecompositor::getBiDecomposition(TernaryBiClique &initBiClique,
                                          const std::vector<size_t> &inputs,
                                          SubnetBuilder &subnetBuilder) {
  if (initBiClique.getOnSet().size() == 1) {
    return makeNetForDNF(*initBiClique.getOnSet().begin(), inputs, 
                         subnetBuilder);
  }
  
  auto starBiCliques = initBiClique.getStarCoverage();

  auto [first, second] = findBaseCoverage(starBiCliques);
  expandBaseCoverage(starBiCliques, first, second);

  TernaryBiClique firstBiClique(initBiClique.getOffSet(),
                                std::move(first.offSet),
                                initBiClique.getVars());

  TernaryBiClique secondBiClique(std::move(initBiClique.getOffSet()),
                                 std::move(second.offSet),
                                 initBiClique.getVars());
  
  firstBiClique.eraseExtraVars(first.vars);
  secondBiClique.eraseExtraVars(second.vars);

  Link lhs = getBiDecomposition(firstBiClique, inputs, subnetBuilder);
  Link rhs = getBiDecomposition(secondBiClique, inputs, subnetBuilder);

  return Link(subnetBuilder.addCell(model::AND, lhs, rhs), false);
}

std::pair<CoverageElement, CoverageElement> 
    BiDecompositor::findBaseCoverage(std::vector<CoverageElement> &stars) {

  auto first = stars.end() - 2;
  auto second = stars.end() - 1;

  auto intersection = popCount(first->vars & second->vars);
  auto merge = popCount(first->vars | second->vars);
 
  for (auto i = stars.begin(); i != (stars.end() - 2); ++i) {
    for (auto j = (i + 1); j != stars.end(); ++j) {
      auto newIntersection = popCount(i->vars & j->vars);
      auto newMerge = popCount(i->vars | j->vars);
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

void BiDecompositor::expandBaseCoverage(std::vector<CoverageElement> &stars,
                                        CoverageElement &first,
                                        CoverageElement &second) {
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
      first.offSet.pushBack(*absorbed->offSet.begin());
    } else {
      second.vars = second.vars | absorbed->vars;
      second.offSet.pushBack(*absorbed->offSet.begin());
    }
    stars.erase(absorbed); 
  }
}

bool BiDecompositor::checkExpanding(uint8_t &difBase, uint8_t &difAbsorbed,
                                    CoverageElement &first,
                                    CoverageElement &second) {
  uint8_t newMerge = popCount(first.vars | second.vars);
  uint8_t newDifBase = newMerge - popCount(first.vars);
  uint8_t newDifAbsorbed = newMerge - popCount(second.vars);
  if (newDifBase < difBase ||
     (newDifBase == difBase && newDifAbsorbed < difAbsorbed)) {
    difBase = newDifBase;
    difAbsorbed = newDifAbsorbed;
    return true;
  }
  return false;
}

Link BiDecompositor::makeNetForDNF(const TernaryVector &vector,
                                     const std::vector<size_t> &inputs,
                                     SubnetBuilder &subnetBuilder) {
  uint32_t bits = vector.getBits();
  uint32_t care = vector.getCare();
  size_t index = std::log2(care - (care & (care - 1)));
  care &= (care - 1);
  bool inv = !((bits >> index) & 1);
  Link prev(inputs[index], inv);
  while (care) {
    index = std::log2(care - (care & (care-1)));
    care &= (care - 1);
    inv = !((bits >> index) & 1);
    prev = 
        Link(subnetBuilder.addCell(model::AND, prev, Link(inputs[index], inv)));
  }
  return prev;
}

} // namespace eda::gate::optimizer2::resynthesis
