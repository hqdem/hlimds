//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "util/bounded_set.h"
#include "util/hash.h"

#include <cstddef>
#include <cstdint>
#include <unordered_set>
#include <vector>

namespace eda::gate::optimizer {

/**
 * @brief Represents a k-feasible cut of a given subnet cell (root).
 */
struct Cut final {
  using Set = util::BoundedSet<model::EntryID>;

  Cut(const model::EntryID rootID, const Set &leafIDs):
      rootID(rootID), leafIDs(leafIDs /* might be empty */) {}

  Cut(const uint16_t k,
      const model::EntryID rootID,
      const std::unordered_set<model::EntryID> &leafIDs,
      const bool isImmutable):
      Cut(rootID, Set(k, leafIDs, isImmutable)) {}

  Cut(const uint16_t k, const model::EntryID rootID, const bool isImmutable):
      Cut(rootID, Set(k, rootID, isImmutable)) {}

  /// Compares this cut w/ the other one.
  bool operator==(const Cut &other) const {
    return rootID == other.rootID && leafIDs == other.leafIDs;
  }

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

  model::EntryID rootID;
  Set leafIDs /* modifiable */;
};

using CutsList = std::vector<Cut>;

} // namespace eda::gate::optimizer

template <>
struct std::hash<eda::gate::optimizer::Cut> {
  size_t operator()(const eda::gate::optimizer::Cut &cut) const noexcept {
    size_t h = std::hash<eda::gate::model::EntryID>{}(cut.rootID);
    eda::util::hash_combine<eda::gate::optimizer::Cut::Set>(h, cut.leafIDs);
    return h;
  }
};
