//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"

#include <queue>
#include <unordered_set>

namespace eda::gate::optimizer2 {

/**
 * @brief Extracts cuts for each cell of the passed subnet.
 * Dominated cuts and cuts with size > k are not added to the result.
 */
class CutExtractor {
public:
  struct Cut;

  using Subnet = model::Subnet;
  using CutsEntries = std::vector<std::unordered_set<uint64_t>>;
  using CutsList = std::vector<Cut>;
  using RawCutsList = std::vector<std::pair<Cut, char>>;

  /// Cut class with its signature and indexes.
  struct Cut {
    Cut() = default;
    Cut(const uint64_t rootEntryIdx,
        const uint64_t sign,
        const std::unordered_set<uint64_t> &entryIdxs):
      rootEntryIdx(rootEntryIdx), signature(sign), entryIdxs(entryIdxs) {};

    /// Unite other cut to current.
    void uniteCut(const Cut &other) {
      for (const auto &entryIdx : other.entryIdxs) {
        entryIdxs.insert(entryIdx);
      }
      signature |= other.signature;
    }

    uint64_t rootEntryIdx {0};
    uint64_t signature {0};
    std::unordered_set<uint64_t> entryIdxs;
  };

  CutExtractor() = delete;
  CutExtractor(const CutExtractor &other) = default;
  CutExtractor &operator=(const CutExtractor &other) = default;
  ~CutExtractor() = default;

  /**
   * @brief Cut extractor constructor.
   * @param subnet Subnet to find cuts.
   * @param k Maximum cut size.
   */
  CutExtractor(const Subnet *subnet,
               const unsigned int k);

  /// Get cuts (entries indexes and signatures) for cell with entryIdx index.
  const CutsList getCuts(uint64_t entryIdx) const;

  /// Get cuts (entries indexes) for cell with entryIdx index.
  CutsEntries getCutsEntries(uint64_t entryIdx) const;

private:
  /// Find all cuts for cell with entryIdx index.
  CutsList findCuts(const uint64_t entryIdx,
                    const uint64_t cellArity,
                    const unsigned int k) const;

  /// Add new cut into addedCuts if it is not dominated and its size < k.
  void addCut(const unsigned int k,
              const uint64_t entryIdx,
              const uint64_t cellArity,
              unsigned long long cutsCombinationIdx,
              RawCutsList &addedCuts,
              const std::vector<std::size_t> &suffCutsCombinationsN) const;

  /// Create and get cut with passed cell.
  Cut getOneElemCut(const uint64_t entryIdx) const;

  /**
   * @brief Check if cut can be added to cuts.
   * 1) Passed cut has < k elements.
   * 2) Passed cut is not dominated by any element from passed cuts.
   * This method also marks elements from cuts as unviable if they are dominated
   * by the passed cut.
   */
  bool checkViable(const Cut &cut, RawCutsList &cuts, unsigned int k) const;

  /// Get only viable cuts (with set flag) from passed cuts.
  CutsList getViable(const RawCutsList &cuts) const;

  /// Check if cut1 dominates cut2.
  bool cutDominates(const Cut &cut1, const Cut &cut2) const;

private:
  const Subnet *subnet;
  std::vector<CutsList> entriesCuts;
};

} // namespace eda::gate::optimizer2
