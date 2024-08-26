//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "util/bounded_set.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <unordered_set>
#include <vector>

namespace eda::gate::optimizer {

/**
 * @brief Represents a k-feasible cut of a given subnet cell (root).
 */
struct Cut final {
  Cut(const size_t rootID,
      const util::BoundedSet<size_t> &entryIDs):
      rootID(rootID), entryIDs{entryIDs} {
    assert(!entryIDs.empty());
  }

  Cut(const uint16_t k,
      const size_t rootID,
      const std::unordered_set<size_t> &entryIDs):
      Cut(rootID, util::BoundedSet<size_t>(k, entryIDs)) {}

  Cut(const uint16_t k,
      const size_t entryID):
      Cut(entryID, util::BoundedSet<size_t>(k, entryID)) {}

  /// Returns the maximum size of the cut.
  uint16_t getK() const {
    return entryIDs.capacity();
  }

  /// Checks whether the cut is trivial.
  bool isTrivial() const {
    return entryIDs.size() == 1 && entryIDs.find(rootID) != entryIDs.end();
  }

  /// Merges the given cut to this one.
  bool merge(const Cut &other) {
    return entryIDs.merge(other.entryIDs);
  }

  /// Checks if this dominates over the other.
  bool dominates(const Cut &other) const;

  const size_t rootID;
  util::BoundedSet<size_t> entryIDs;
};

using CutsList = std::vector<Cut>;

} // namespace eda::gate::optimizer
