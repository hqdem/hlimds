//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"

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
  using CutsCells = std::vector<std::unordered_set<std::size_t>>;
  using CutsList = std::vector<Cut>;
  using RawCutsList = std::vector<std::pair<Cut, char>>;

  /**
   * @brief Cut class with its signature and indexes.
   */
  struct Cut {
    Cut() = default;
    Cut(const unsigned long long sign,
        const std::unordered_set<uint64_t> &cellIdxs):
      signature(sign), cellIdxs(cellIdxs) {};

    unsigned long long signature {0};
    std::unordered_set<uint64_t> cellIdxs;
  };

  CutExtractor() = delete;
  CutExtractor(const CutExtractor &other) = default;
  CutExtractor(CutExtractor &&other) = default;
  CutExtractor &operator=(const CutExtractor &other) = default;
  CutExtractor &operator=(CutExtractor &&other) = default;
  ~CutExtractor() = default;

  /**
   * @brief Cut extractor constructor.
   * Finds cuts for each cell of passed subnet.
   * @param subnet Subnet to find cuts.
   * @param k Maximum cut size.
   */
  CutExtractor(const Subnet &subnet, const unsigned int k);

  /// Get cuts (with indexes and signatures) for cell with cellIdx index.
  const CutsList &getCuts(uint64_t cellIdx) const;

  /**
   * @brief Get cuts cells (cells indexes in cuts) for cell with cellIdx index.
   */
  CutsCells getCutsCells(uint64_t cellIdx) const;

private:
  /**
   * @brief Find all cuts for cell with cellIdx index.
   */
  CutsList findCuts(const Subnet &subnet, const std::size_t cellIdx,
                    const uint64_t cellArity, const unsigned int k) const;

  /// Add new cut into addedCuts if it is not dominated and its size < k.
  void addCut(const Subnet &subnet, const unsigned int k,
              const std::size_t cellIdx, const uint64_t cellArity,
              unsigned long long cutsCombinationIdx, RawCutsList &addedCuts,
              const std::vector<std::size_t> &suffCutsCombinationsN) const;

  /// Unite cut2 to cut1.
  void uniteCut(Cut &cut1, const Cut &cut2) const;

  /// Create and get cut with passed cell.
  Cut getOneElemCut(const Subnet &subnet, const std::size_t cellIdx) const;

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
  std::vector<CutsList> cellsCuts;
};

} // namespace eda::gate::optimizer2
