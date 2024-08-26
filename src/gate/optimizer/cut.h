//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "util/bounded_set.h"

#include <cstddef>
#include <cstdint>
#include <unordered_set>
#include <vector>

namespace eda::gate::optimizer {

/**
 * @brief Represents a k-feasible cut of a given subnet cell (root).
 */
struct Cut final {
  using Set = util::BoundedSet<size_t>;

  Cut(const size_t rootID, const Set &leafIDs):
      rootID(rootID), leafIDs(leafIDs /* might be empty */) {}

  Cut(const uint16_t k,
      const size_t rootID,
      const std::unordered_set<size_t> &leafIDs):
      Cut(rootID, Set(k, leafIDs)) {}

  Cut(const uint16_t k, const size_t rootID):
      Cut(rootID, Set(k, rootID)) {}

  /// Returns the maximum size of the cut.
  uint16_t getK() const {
    return leafIDs.capacity();
  }

  /// Checks whether the cut is trivial.
  bool isTrivial() const {
    return leafIDs.size() == 1 && leafIDs.find(rootID) != leafIDs.end();
  }

  /// Merges the given cut to this one.
  bool merge(const Cut &other) {
    return leafIDs.merge(other.leafIDs);
  }

  /// Checks if this cut dominates over (is subnet of) the other.
  bool dominates(const Cut &other) const {
    return leafIDs.size() < other.leafIDs.size()
        && other.leafIDs.contains(leafIDs);
  }

  const size_t rootID;
  Set leafIDs /* modifiable */;
};

using CutsList = std::vector<Cut>;

} // namespace eda::gate::optimizer
