//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "kitty/kitty.hpp"

#include <initializer_list>
#include <set>
#include <vector>

namespace eda::gate::optimizer2::resynthesis {

/// Counts the number of 1 bits in the bitset.
template<typename T>
uint8_t popCount(T number) {
  uint8_t count { 0 };
  for(; number; ++count) {
    number &= (number - 1);
  }
  return count;
}

/**
 * \brief Represents a vector, vars in which can have only three values:
 * 0, 1 and "don't care" (0 or 1).
 */
class TernaryVector final {
public:

  TernaryVector() = default;

  TernaryVector(uint32_t bits, uint32_t care) : bits(bits), care(care) { }

  /// Returns orthogonal bits (two vectors are orthogonal to each other by bit,
  /// if in one vector the bit has the value 0, and in the other the value 1.
  uint32_t orthogonality(const TernaryVector &rhs) const;

  uint32_t getBits() const {
    return bits;
  }

  uint32_t& getBits() {
    return bits;
  }

  uint32_t getCare() const {
    return care;
  }

  uint32_t& getCare() {
    return care;
  }

private:

  uint32_t bits;
  uint32_t care;
};

bool operator==(const TernaryVector &lhs, const TernaryVector &rhs);

/**
 * \brief Represent a matrix, whose rows is ternary vectors.
 */
class TernaryMatrix final {
public:

  TernaryMatrix() = default;

  TernaryMatrix(const std::initializer_list<TernaryVector> &init)
      : rows(init) { }

  TernaryMatrix(const std::vector<TernaryVector> &rows) : rows(rows) { }

  bool empty() const {
    return rows.empty();
  }

  size_t size() const {
    return rows.size();
  }

  std::vector<TernaryVector>::const_iterator begin() const {
    return rows.begin();
  }

  std::vector<TernaryVector>::const_iterator end() const {
    return rows.end();
  }

  void pushBack(const TernaryVector &vector) {
    rows.push_back(vector);
  }

  void mergeVectors(uint32_t vars);

  void eraseExtraVars(uint32_t vars);

  friend bool operator==(const TernaryMatrix &lhs, const TernaryMatrix &rhs);

private:

  void openVectors(uint32_t vars);

  static void getAbsorbedVectors(uint32_t pos, uint32_t bits,
                                 std::set<uint32_t> &allAbsorbedVectors);

  static bool checkMerge(const std::vector<uint32_t> &allSourceVectors,
                         const std::set<uint32_t> &allAbsorbedVectors);

  std::vector<TernaryVector> rows;
};

/**
 * \brief Represents a complete bipartite graph (bi-clique), whose nodes
 * are ternary vectors. Each part of graph constitutes ternary matrix.
 * TODO: Implement Minato-Morreale algorithm to use ISOP instead of
 * ternary matrices.
 */
class TernaryBiClique final {
public:

  using KittyTT    = kitty::dynamic_truth_table;
  using NormalForm = std::set<uint32_t>;

  struct CoverageElement final {
    TernaryMatrix offSet;
    uint32_t vars;
  };

  TernaryBiClique(const KittyTT &func, const KittyTT &care);

  TernaryBiClique(TernaryMatrix onSet, TernaryMatrix offSet, uint32_t vars);

  std::vector<CoverageElement> getStarCoverage() const;

  void eraseExtraVars(uint32_t vars);

  TernaryMatrix& getOffSet() {
    return offSet;
  }

  TernaryMatrix& getOnSet() {
    return onSet;
  }

  uint32_t getVars() const {
    return vars;
  }

private:

  uint32_t findVars(const TernaryVector &vector) const;

  static void multiplyDisjuncts(NormalForm &dnf, uint32_t disjunct);

  uint32_t vars;
  TernaryMatrix onSet;
  TernaryMatrix offSet;
};

bool operator==(const TernaryBiClique::CoverageElement &lhs,
                const TernaryBiClique::CoverageElement &rhs);

} // namespace eda::gate::optimizer2::resynthesis
