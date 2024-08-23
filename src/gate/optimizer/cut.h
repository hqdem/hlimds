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
 * @brief Represents a cut of a given cell.
 */
struct Cut final {
  Cut(const uint16_t k,
      const size_t rootEntryIdx,
      const BoundedSet<size_t> &entryIdxs):
      k(k), rootEntryIdx(rootEntryIdx), entryIdxs{entryIdxs} {}
  
  Cut(const size_t rootEntryIdx,
      const std::unordered_set<size_t> &entryIdxs):
      Cut(entryIdxs.size(), rootEntryIdx, entryIdxs) {}

  Cut(const uint16_t k,
      const size_t entryIdx):
      Cut(k, entryIdx, BoundedSet<size_t>(k, entryIdx)) {}

  Cut(const size_t entryIdx):
      Cut(1, entryIdx) {}

  /// Checks whether the cut is trivial.
  bool isTrivial() const {
    return entryIdxs.size() == 1 &&
           entryIdxs.find(rootEntryIdx) != entryIdxs.end();
  }

  /// Merges the given cut to this one.
  bool merge(const Cut &other) {
    return entryIdxs.merge(other.entryIdxs);
  }

  /// Checks if this dominates the other.
  bool dominates(const Cut &other) const;

  const uint16_t k; 
  size_t rootEntryIdx;
  BoundedSet<size_t> entryIdxs;
};

using CutsList = std::vector<Cut>;

} // namespace eda::gate::optimizer
