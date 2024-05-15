//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"

#include <queue>
#include <unordered_set>

namespace eda::gate::optimizer {

/**
 * @brief Extracts cuts for each cell of the passed subnet.
 * Dominated cuts and cuts with size > k are not added to the result.
 */
class CutExtractor {
public:
  struct Cut;

  using Subnet = model::Subnet;
  using LinkList = Subnet::LinkList;
  using SubnetBuilder = model::SubnetBuilder;
  using CutsEntries = std::vector<std::unordered_set<size_t>>;
  using CutsList = std::vector<Cut>;
  using RawCutsList = std::vector<std::pair<Cut, char>>;
  using Entry = model::Subnet::Entry;

  /// Cut class with its signature and indexes.
  struct Cut {
    Cut() = default;
    Cut(const size_t rootEntryIdx,
        const uint64_t sign,
        const std::unordered_set<size_t> &entryIdxs):
      rootEntryIdx(rootEntryIdx), signature(sign), entryIdxs(entryIdxs) {};

    /// Unite other cut to current.
    void uniteCut(const Cut &other) {
      for (const auto &otherEntryIdx : other.entryIdxs) {
        entryIdxs.insert(otherEntryIdx);
      }
      signature |= other.signature;
    }

    size_t rootEntryIdx {0};
    uint64_t signature {0};
    std::unordered_set<size_t> entryIdxs;
  };

  CutExtractor() = delete;
  CutExtractor(const CutExtractor &other) = default;
  CutExtractor &operator=(const CutExtractor &other) = default;
  ~CutExtractor() {
    if (saveSubnetEntries) {
      delete saveSubnetEntries;
    }
  }

  /**
   * @brief Cut extractor constructor.
   * @param subnet Subnet to find cuts.
   * @param k Maximum cut size.
   */
  CutExtractor(const Subnet *subnet, const unsigned int k);

  CutExtractor(SubnetBuilder *builder, const unsigned k);

  /// Get cuts (entries indexes and signatures) for cell with entryIdx index.
  const CutsList getCuts(size_t entryIdx) const;

  /// Get cuts (entries indexes) for cell with entryIdx index.
  CutsEntries getCutsEntries(size_t entryIdx) const;

  /// Recomputes cuts for passed entry. All entries used by passed entry must be
  /// computed.
  void recomputeCuts(size_t entryIdx);

private:
  LinkList getLinks(size_t entryID) const;

  /// Find all cuts for cell with entryIdx index.
  void findCuts(const size_t entryIdx);

  /// Add new cut into addedCuts if it is not dominated and its size < k.
  void addCut(
      const size_t entryIdx,
      unsigned long long cutsCombinationIdx,
      RawCutsList &addedCuts,
      const std::vector<size_t> &suffCutsCombinationsN) const;

  /// Create and get cut with passed cell.
  Cut getOneElemCut(const size_t entryIdx) const;

  /// Add only viable cuts (with set flag) to the cuts storage.
  void addViableCuts(const RawCutsList &cuts, const size_t entryIdx);

  /**
   * @brief Check if the passed cut is not dominated by any element from the
   * passed cuts.
   * This method also marks elements from the passed cuts as unviable if they
   * are dominated by the passed cut.
   */
  bool cutNotDominated(const Cut &cut, RawCutsList &cuts) const;

  /// Check if cut1 dominates cut2.
  bool cutDominates(const Cut &cut1, const Cut &cut2) const;

  /// Compute signature of the combination of the passed entry links.
  uint64_t getNewCutSign(
      unsigned long long cutsCombinationIdx,
      const LinkList &entryLinks,
      const std::vector<std::size_t> &suffCutsCombN) const;

private:
  const Subnet *subnet;
  const model::Array<Entry> *saveSubnetEntries; //TODO: fixme

  const SubnetBuilder *builder;

  unsigned k;
  std::vector<CutsList> entriesCuts;
};

} // namespace eda::gate::optimizer
