//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer2/resynthesis/ternary_bi_clique.h"
#include "util/assert.h"

#include <algorithm>

namespace eda::gate::optimizer2::resynthesis {

using CoverageElement = TernaryBiClique::CoverageElement;
using KittyTT         = TernaryBiClique::KittyTT;
using NormalForm      = TernaryBiClique::NormalForm;

TernaryVector::TernaryVector() : bits(0), care(0) { };

TernaryVector::TernaryVector(uint32_t bits, size_t vars)
    : bits(bits), care((1 << vars) - 1) { }

TernaryVector::TernaryVector(uint32_t bits, uint32_t care)
    : bits(bits), care(care) { }

uint32_t TernaryVector::orthogonality(const TernaryVector &rhs) const {
  return (bits & care & rhs.care) ^ (rhs.bits & care & rhs.care);
}

uint32_t TernaryVector::getBits() const {
  return bits;
}

uint32_t& TernaryVector::getBits() {
  return bits;
}

uint32_t TernaryVector::getCare() const {
  return care;
}

uint32_t& TernaryVector::getCare() {
  return care;
}

bool operator==(const TernaryVector &lhs, const TernaryVector &rhs) {
  return (lhs.getBits() == rhs.getBits()) && (lhs.getCare() == rhs.getCare());
}

TernaryMatrix::TernaryMatrix(const std::initializer_list<TernaryVector> &init) 
  : rows(init) { }

TernaryMatrix::TernaryMatrix(const std::vector<TernaryVector> &rows) 
  : rows(rows), merged(true) { }

bool TernaryMatrix::empty() const {
  return rows.empty();
}

size_t TernaryMatrix::size() const {
  return rows.size();
}

std::vector<TernaryVector>::const_iterator TernaryMatrix::begin() const {
  return rows.begin();
}

std::vector<TernaryVector>::const_iterator TernaryMatrix::end() const {
  return rows.end();
}

void TernaryMatrix::pushBack(const TernaryVector &vector) {
  rows.push_back(vector);
}

void TernaryMatrix::eraseExtraVars(uint32_t vars) { 
  for (TernaryVector &vector : rows) {
    vector.getBits() &= vars;
    vector.getCare() &= vars; 
  }
  openVectors(vars);
  mergeVectors(vars);
}

void TernaryMatrix::openVectors(uint32_t vars) {
  std::set<uint32_t> allAbsorbedVectors;

  for (const TernaryVector &vector : rows) {
    getAbsorbedVectors(vector.getCare() ^ vars, vector.getBits(),
                       allAbsorbedVectors);
  }

  rows.resize(allAbsorbedVectors.size());

  std::transform(allAbsorbedVectors.begin(), allAbsorbedVectors.end(),
                 rows.begin(),
                 [care = vars](uint32_t bits){
                  return TernaryVector(bits, care);
                 });
  merged = false;
}

void TernaryMatrix::mergeVectors(uint32_t vars) {
  if (rows.size() <= 1 || merged) {
    return;
  }

  std::vector<uint32_t> allSourceVectors(rows.size());
  std::set<uint32_t> allAbsorbedVectors;

  std::transform(rows.begin(), rows.end(), allSourceVectors.begin(), 
                 [](const TernaryVector &vector) {
                    return vector.getBits(); 
                 });

  auto base = rows.begin();
  auto absorbed = base + 1;
  auto end = rows.end();

  while (base != end - 1) {
    uint32_t orthBits = base->orthogonality(*absorbed);
    if (popCount(orthBits) == 1) {
      TernaryVector newBase = *base;
      newBase.getCare() ^= orthBits;
      allAbsorbedVectors.clear();
      getAbsorbedVectors(newBase.getCare() ^ vars, base->getBits(),
                         allAbsorbedVectors);
      if (checkMerge(allSourceVectors, allAbsorbedVectors)) {
        TernaryVector newAbsorbed = *absorbed;
        *base = newBase;
        for (const auto &vector : allAbsorbedVectors) {
          auto it = std::find_if(absorbed, rows.end(), 
                                 [vector](const auto &row) {
                                   return row.getBits() == vector;
                                 });
          if (it != rows.end()) {
            if (*it == newAbsorbed && it != rows.end() - 1) {
              newAbsorbed = *(it + 1);
            }
            rows.erase(it);
          }
        }
        end = rows.end();
        base = std::find(rows.begin(), rows.end(), newBase);
        absorbed = std::find(rows.begin(), rows.end(), newAbsorbed);
      } else {
        ++absorbed;
      }
    } else {
      ++absorbed;
    }
    if (absorbed == end && base != end - 1) {
      ++base;
      absorbed = base + 1;
    }
  }

  for (base = rows.begin(); base != rows.end() - 1; ++base) {
    for (absorbed = base + 1; absorbed != rows.end(); ++absorbed) {
      allAbsorbedVectors.clear();
      uint32_t orthBits = base->orthogonality(*absorbed);
      if (orthBits) {
        TernaryVector newAbsorbed = *absorbed;
        newAbsorbed.getCare() ^= orthBits;
        getAbsorbedVectors(newAbsorbed.getCare() ^ vars, newAbsorbed.getBits(), 
                           allAbsorbedVectors);
        if (checkMerge(allSourceVectors, allAbsorbedVectors)) {
          *absorbed = newAbsorbed;
        }
      }
    }
  }
  merged = true;
}

void TernaryMatrix::getAbsorbedVectors(uint32_t pos, uint32_t bits,
    std::set<uint32_t> &allAbsorbedVectors) {
  if (!popCount(pos)) {
    allAbsorbedVectors.insert(bits);
  } else {
    uint32_t newPos = pos & (pos - 1);
    getAbsorbedVectors(newPos, bits | (pos - newPos), allAbsorbedVectors);
    getAbsorbedVectors(newPos, bits & ~(pos - newPos), allAbsorbedVectors);
  }
}

bool TernaryMatrix::checkMerge(const std::vector<uint32_t> &allSourceVectors,
                                 const std::set<uint32_t> &allAbsorbedVectors) {
  return std::includes(allSourceVectors.begin(), allSourceVectors.end(),
                       allAbsorbedVectors.begin(), allAbsorbedVectors.end());
}


bool operator==(const TernaryMatrix &lhs, const TernaryMatrix &rhs) {
  return lhs.rows == rhs.rows;
}

TernaryBiClique::TernaryBiClique(const KittyTT &func, const KittyTT &care) {

  const size_t fSize = func.num_vars();
  const size_t cSize = care.num_vars();

  uassert(fSize == cSize, "func and care have different sizes");

  vars = ((1u << func.num_vars()) - 1u);

  for (uint32_t i = 0; i < func.num_bits(); ++i) {
    if (get_bit(care, i)) {
      get_bit(func, i) ? onSet.pushBack({i, fSize})
          : offSet.pushBack({i, fSize});
    }
  }

  onSet.mergeVectors(vars);
  offSet.mergeVectors(vars);
}

TernaryBiClique::TernaryBiClique(TernaryMatrix onSet, TernaryMatrix offSet,
                                 uint32_t vars) 
  : vars(vars), onSet(std::move(onSet)), offSet(std::move(offSet)) { }

void TernaryBiClique::eraseExtraVars(uint32_t vars) {
  if (vars == this->vars) {
    return;
  }
  this->onSet.eraseExtraVars(vars);
  this->offSet.eraseExtraVars(vars);
}

std::vector<CoverageElement> TernaryBiClique::getStarCoverage() const {

  uassert(!onSet.empty() && !offSet.empty(),
      "There are not edges in the bi-clique");

  std::vector<CoverageElement> coverage;
  coverage.reserve(onSet.size());
  for (const auto &vector : onSet) {
    coverage.push_back({{vector}, findVars(vector)});
  }
  return coverage;
}

TernaryMatrix& TernaryBiClique::getOffSet() {
  return offSet;
}

TernaryMatrix& TernaryBiClique::getOnSet() {
  return onSet;
}

uint32_t TernaryBiClique::getVars() const {
  return vars;
}

uint32_t TernaryBiClique::findVars(const TernaryVector &vector) const {

  NormalForm cnf;
  for (const auto &v : offSet) {
    uint32_t q = vector.orthogonality(v);
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

  // Finds weight of the bi-clique.
  auto vars = *std::min_element(dnf.begin(), dnf.end(), [](auto lhs, auto rhs) {
    return popCount(lhs) < popCount(rhs);
  });

  return vars;
}

void TernaryBiClique::multiplyDisjuncts(NormalForm &dnf, uint32_t disjunct) {
  NormalForm newDNF;
  if (popCount(disjunct) == 1) {
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

bool operator==(const CoverageElement &lhs, const CoverageElement &rhs) {
  return (lhs.offSet == rhs.offSet) && (lhs.vars == rhs.vars); 
}

} // namespace eda::gate::optimizer2::resynthesis
