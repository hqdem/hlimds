//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"
#include "gate/optimizer/cut.h"

#include <cstddef>
#include <cstdint>
#include <unordered_set>
#include <vector>

namespace eda::gate::optimizer {

/**
 * @brief Extracts all cuts of the bounded size for each cell of a subnet.
 *        Dominated cuts are not added to the result.
 */
class CutExtractor final {
public:
  using Subnet = model::Subnet;
  using Link = Subnet::Link;
  using SubnetBuilder = model::SubnetBuilder;
  using CutsEntries = std::vector<Cut::Set>;
  using RawCutsList = std::vector<std::pair<Cut, char>>;

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
   * @param builder Subnet builder to find cuts in.
   * @param k Maximum cut size.
   * @param extractNow Extracts cuts right now.
   */
  CutExtractor(const SubnetBuilder *builder,
               const uint16_t k,
               const bool extractNow);

  /// Gets cuts (entries indexes and signatures) for cell with entryID index.
  const CutsList getCuts(const size_t entryID) const {
    return entriesCuts[entryID];
  }

  /// Gets cuts (entries indexes) for cell with entryID index.
  CutsEntries getCutsEntries(const size_t entryID) const;

  /// Recomputes cuts for passed entry.
  /// All entries used by passed entry must be computed.
  void recomputeCuts(const size_t entryID);

private:
  const Link *getLinks(
      const size_t entryID, Link *links, uint16_t &nLinks) const {
    return subnet ? subnet->getLinks(entryID, links, nLinks)
                  : builder->getLinks(entryID, links, nLinks);
  }

  /// Finds all cuts for cell with entryID index.
  void findCuts(const size_t entryID);

  /// Adds new cut into addedCuts if it is not dominated and its size < k.
  void addCut(
      const size_t entryID,
      const Link links[],
      const uint16_t nLinks,
      uint64_t cutsCombinationIdx,
      RawCutsList &addedCuts,
      const std::vector<size_t> &suffCutsCombinationsN) const;

  /// Adds only viable cuts (with set flag) to the cuts storage.
  void addViableCuts(const RawCutsList &cuts, const size_t entryID);

  /**
   * @brief Checks if the cut is not dominated by any element from the
   * passed cuts.
   * This method also marks elements from the passed cuts as unviable if they
   * are dominated by the passed cut.
   */
  bool cutNotDominated(const Cut &cut, RawCutsList &cuts) const;

private:
  const Subnet *subnet;
  const SubnetBuilder *builder;
  std::vector<CutsList> entriesCuts;
  uint16_t k;
};

} // namespace eda::gate::optimizer
