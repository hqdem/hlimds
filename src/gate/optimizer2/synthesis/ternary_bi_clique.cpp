//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer2/synthesis/ternary_bi_clique.h"
#include "util/assert.h"

// Removes a bit from the number by shifting the left bits to the right
#define REMOVE_BIT(bits, idx)\
  bits = ((bits >> 1) & (0xffffffff << idx)) +\
         (bits & (0x00000000 | ((1u << idx) - 1)));\

namespace eda::gate::optimizer2::synthesis {

using Coverage = TernaryBiClique::Coverage;

TernaryBiClique::TernaryBiClique(const KittyTT &func, const KittyTT &care)
    : indices((1u << func.num_vars()) - 1u),
      onSet(kitty::isop(func & care)),
      offSet(kitty::isop(~func & care)) {

  auto fSize = func.num_vars();
  auto cSize = func.num_vars();
      
  uassert(fSize == cSize, "func and care have different sizes");
  uassert(fSize <= 32, "Too many inputs");

  for (size_t i{0}; i < fSize; ++i) {
    inputs.push_back(Link(i));
  }
}

TernaryBiClique::TernaryBiClique(ISOP onSet, ISOP offSet, uint32_t indices,
                                 LinkList inputs, uint32_t oldIndices)
  : indices(indices),
    inputs(std::move(inputs)),
    onSet(std::move(onSet)),
    offSet(std::move(offSet)) { 

  if (oldIndices == this->indices) {
    return;
  }

  eraseExtraInputs(this->onSet, oldIndices);
  eraseExtraInputs(this->offSet, oldIndices);

  uint32_t idxs = oldIndices ^ this->indices;
  size_t i{0};
  for(; idxs; idxs &= (idxs - 1)) {
    size_t idx = std::log2(idxs - (idxs & (idxs - 1))) - i;
    REMOVE_BIT(this->indices, idx)
    ++i;
    this->inputs.erase(this->inputs.begin() + idx);
  }
}

std::vector<Coverage> TernaryBiClique::getStarCoverage() const {

  uassert(!onSet.empty() && !offSet.empty(),
      "There are not edges in the bi-clique");

  std::vector<Coverage> coverage;
  coverage.reserve(onSet.size());
  for (const Cube &vector : onSet) {
    coverage.push_back({{vector}, findIndices(vector)});
  }

  return coverage;
}

uint32_t TernaryBiClique::findIndices(const Cube &vector) const {

  NormalForm cnf;
  for (const Cube &v : offSet) {
    uint32_t q = findCubeOrthogonality(vector, v);
    cnf.insert(q);
  }

  // Absorption law.
  auto i = cnf.begin();
  auto j = std::next(i);
  auto end = cnf.end();
  while (i != std::next(end, -1)) {
    if (((*i & *j) == *i) || ((*i & *j) == *j)) {
      uint32_t newI;
      uint32_t newJ;
      const auto it = ((*i & *j) == *j ? i : j);
      if (it == j) { // Absorbs j.
        newI = *i;
        if (j == std::next(end, -1)) {
          newJ = *j;
        } else {
          newJ = *(std::next(j)); 
        }
      } else { // Absorbs i.
        newI = *next(i);
        if (i == std::next(end, -2)) {
          newJ = *j;
        } else {
          newJ = *(std::next(i, 2));
        }
      }
      cnf.erase(it);
      end = cnf.end();
      i = cnf.find(newI);
      j = cnf.find(newJ);
    } else {
      ++j;
    }
    if (j == end && i != (std::next(end, -1))) {
      ++i;
      j = std::next(i);
    }
  }

  // Opening brackets and forms dnf.
  NormalForm dnf;
  uint32_t tmp = *(cnf.begin());
  while(tmp) {
    uint32_t prev =  tmp;
    tmp &= (tmp - 1);
    dnf.insert(prev - tmp);
  }

  for (auto it = std::next(cnf.begin()); it != cnf.end(); ++it) {
    multiplyDisjuncts(dnf, *it);
  }

  auto foundIndices = *std::min_element(dnf.begin(), dnf.end(),
      [](auto lhs, auto rhs) {
        return __builtin_popcount(lhs) < __builtin_popcount(rhs);
      });

  return foundIndices;
}

void TernaryBiClique::multiplyDisjuncts(NormalForm &dnf, uint32_t disjunct) {
  NormalForm newDNF;
  if (__builtin_popcount(disjunct) == 1) {
    for (auto conjunct : dnf) {
      conjunct |= disjunct;
      newDNF.insert(conjunct);
    }
  } else {
    while(disjunct) {
      NormalForm oldDNF = dnf;
      multiplyDisjuncts(oldDNF, disjunct - (disjunct & (disjunct - 1)));
      newDNF.merge(oldDNF);
      disjunct &= (disjunct - 1);
    }
  }
  dnf = newDNF;
}

void TernaryBiClique::eraseExtraInputs(ISOP &isop, uint32_t oldIndices) {
  uint32_t idxs = oldIndices ^ this->indices;
  size_t i{0};
  for(; idxs; idxs &= (idxs - 1)) {
    size_t idx = std::log2(idxs - (idxs & (idxs - 1))) - i;
    for (Cube &cube : isop) {
      REMOVE_BIT(cube._mask, idx)
      REMOVE_BIT(cube._bits, idx)
    }
    ++i;
  }
  KittyTT tt(__builtin_popcount(indices));
  kitty::create_from_cubes(tt, isop);
  isop = kitty::isop(tt);
}

} // namespace eda::gate::optimizer2::synthesis
