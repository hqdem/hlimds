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

using BoundGNet       = BiDecompositor::BoundGNet;
using CoverageElement = BiDecompositor::CoverageElement;
using GateId          = BiDecompositor::GateId;

BoundGNet BiDecompositor::run(const KittyTT &func, const KittyTT &care) {
  auto net = std::make_shared<GNet>();
  GateIdList inputs;
  for (size_t i = 0; i < func.num_vars(); ++i) {
    inputs.push_back(net->addIn());
  }
  TernaryBiClique initBiClique(func, care);
  GateId output = net->addOut(getBiDecomposition(initBiClique, inputs, *net));
  net->sortTopologically();
  return BoundGNet{net, inputs, {output}};
}

GateId BiDecompositor::getBiDecomposition(TernaryBiClique &initBiClique,
                                          const GateIdList &inputs, GNet &net) {
  if (initBiClique.getOnSet().size() == 1) {
    return makeNetForDNF(*initBiClique.getOnSet().begin(), inputs, net);
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

  auto id1 = getBiDecomposition(firstBiClique, inputs, net);
  auto id2 = getBiDecomposition(secondBiClique, inputs, net);

  return net.addNot(net.addAnd(id1, id2));
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

GateId BiDecompositor::makeNetForDNF(const TernaryVector &vector,
                                     const GateIdList &inputs, GNet &net) {
  uint32_t bits = vector.getBits();
  uint32_t care = vector.getCare();
  size_t index = std::log2(care - (care & (care - 1)));
  care &= (care - 1);
  GateId prev = ((bits >> index) & 1) ?
      inputs[index] : net.addNot(inputs[index]);
  while (care) {
    index = std::log2(care - (care & (care-1)));
    care &= (care - 1);
    prev = net.addAnd(prev, 
        ((bits >> index) & 1) ? inputs[index] : net.addNot(inputs[index]));
  }
  return prev;
}

} // namespace eda::gate::optimizer2::resynthesis
