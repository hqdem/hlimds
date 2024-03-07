//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"

#include <kitty/kitty.hpp>

#include <cstdint>
#include <set>
#include <vector>

namespace eda::gate::optimizer2::synthesis {

/**
 * \brief Represents a complete bipartite graph (bi-clique), whose nodes
 * are cubes. Each part of graph constitutes ISOP (Irredundant Sum-Of-Poducts).
 */
class TernaryBiClique final {
public:

  using Cube       = kitty::cube;
  using ISOP       = std::vector<Cube>;
  using KittyTT    = kitty::dynamic_truth_table;
  using Link       = model::Subnet::Link;
  using LinkList   = model::Subnet::LinkList;
  using NormalForm = std::set<uint32_t>;

  TernaryBiClique(const KittyTT &func, const KittyTT &care);

  TernaryBiClique(ISOP onSet, ISOP offSet, uint32_t indices,
                  LinkList inputs, uint32_t oldIndices);

  uint32_t getIndices() const {
    return indices;
  }

  LinkList& getInputs() {
    return inputs;
  }

  ISOP& getOnSet() {
    return onSet;
  }

  ISOP& getOffSet() {
    return offSet;
  }

  struct Coverage final {
    ISOP offSet;
    uint32_t vars;
  };

  /// Generates stars bi-cliques (a bipartite graph, in which one part
  /// has only one node) from current bi-clique.
  std::vector<Coverage> getStarCoverage() const;

private:

  static uint32_t findCubeOrthogonality(const Cube &lhs, const Cube &rhs) {
    return (lhs._bits & lhs._mask & rhs._mask) ^
           (rhs._bits & lhs._mask & rhs._bits);
  }

  uint32_t findIndices(const Cube &vector) const;

  static void multiplyDisjuncts(NormalForm &dnf, uint32_t disjunct);

  void eraseExtraInputs(ISOP &isop, uint32_t oldIndices);

  uint32_t indices;
  LinkList inputs;

  ISOP onSet;
  ISOP offSet;
};

inline bool operator==(const TernaryBiClique::Coverage &lhs,
                       const TernaryBiClique::Coverage &rhs) {
  return (lhs.offSet == rhs.offSet) && (lhs.vars == rhs.vars);
}

} // namespace eda::gate::optimizer2::synthesis
