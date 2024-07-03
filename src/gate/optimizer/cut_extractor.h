//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"

#include <cstddef>
#include <unordered_set>
#include <vector>

namespace eda::gate::optimizer {

/**
 * @brief Extracts cuts for each cell of the passed subnet.
 * Dominated cuts and cuts with size > k are not added to the result.
 */
class CutExtractor final {
public:
  struct Cut;

  using Subnet = model::Subnet;
  using LinkList = Subnet::LinkList;
  using SubnetBuilder = model::SubnetBuilder;
  using CutsEntries = std::vector<std::unordered_set<size_t>>;
  using CutsList = std::vector<Cut>;
  using RawCutsList = std::vector<std::pair<Cut, char>>;
  using Entry = model::Subnet::Entry;

  /// Cut class with its signature and indices.
  struct Cut {
    Cut() = default;
    Cut(const size_t rootEntryIdx,
        const uint64_t sign,
        const std::unordered_set<size_t> &entryIdxs):
      rootEntryIdx(rootEntryIdx), signature(sign), entryIdxs(entryIdxs) {}

    /// Unites other cut to current.
    void uniteCut(const Cut &other) {
      for (const auto &otherEntryIdx : other.entryIdxs) {
        entryIdxs.insert(otherEntryIdx);
      }
      signature |= other.signature;
    }

    size_t rootEntryIdx{0};
    uint64_t signature{0};
    std::unordered_set<size_t> entryIdxs;
  };

  CutExtractor() = delete;
  CutExtractor(const CutExtractor &other) = default;
  CutExtractor &operator=(const CutExtractor &other) = default;

  /**
   * @brief Constructs a cut extractor for the given subnet.
   *
   * @param subnet Subnet to find cuts.
   * @param k Maximum cut size.
   */
  CutExtractor(const Subnet *subnet, const uint16_t k);

  /**
   * @brief Constructs a cut extractor for the given subnet builder.
   *
   * @param builder Subnet builder to find cuts.
   * @param k Maximum cut size.
   * @param extractNow Extracts cuts right now.
   */
  CutExtractor(const SubnetBuilder *builder,
               const uint16_t k,
               const bool extractNow);

  /// Gets cuts (entries indexes and signatures) for cell with entryIdx index.
  const CutsList getCuts(const size_t entryIdx) const;

  /// Gets cuts (entries indexes) for cell with entryIdx index.
  CutsEntries getCutsEntries(const size_t entryIdx) const;

  /// Recomputes cuts for passed entry.
  /// All entries used by passed entry must be computed.
  void recomputeCuts(const size_t entryIdx);

private:
  LinkList getLinks(const size_t entryID) const;

  /// Finds all cuts for cell with entryIdx index.
  void findCuts(const size_t entryIdx);

  /// Adds new cut into addedCuts if it is not dominated and its size < k.
  void addCut(
      const size_t entryIdx,
      uint64_t cutsCombinationIdx,
      RawCutsList &addedCuts,
      const std::vector<size_t> &suffCutsCombinationsN) const;

  /// Creates and gets cut with passed cell.
  Cut getOneElemCut(const size_t entryIdx) const;

  /// Adds only viable cuts (with set flag) to the cuts storage.
  void addViableCuts(const RawCutsList &cuts, const size_t entryIdx);

  /**
   * @brief Checks if the passed cut is not dominated by any element from the
   * passed cuts.
   * This method also marks elements from the passed cuts as unviable if they
   * are dominated by the passed cut.
   */
  bool cutNotDominated(const Cut &cut, RawCutsList &cuts) const;

  /// Checks if cut1 dominates cut2.
  bool cutDominates(const Cut &cut1, const Cut &cut2) const;

  /// Computes signature of the combination of the passed entry links.
  uint64_t getNewCutSign(
      uint64_t cutsCombinationIdx,
      const LinkList &entryLinks,
      const std::vector<size_t> &suffCutsCombN) const;

private:
  const Subnet *subnet;
  const SubnetBuilder *builder;

  uint16_t k;
  std::vector<CutsList> entriesCuts;
};

} // namespace eda::gate::optimizer
