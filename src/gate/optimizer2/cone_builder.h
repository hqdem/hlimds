//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer2/cut_extractor.h"

#include <functional>
#include <stack>

namespace eda::gate::optimizer2 {

/// Builds cone as structure with Subnet and entries mapping.
class ConeBuilder {
public:
  using Subnet = model::Subnet;
  using SubnetID = model::SubnetID;
  using LinkList = Subnet::LinkList;
  using SubnetBuilder = model::SubnetBuilder;
  using Cut = optimizer2::CutExtractor::Cut;

  using EntryMap = std::unordered_map<uint64_t, uint64_t>;
  using EntryCheckFunc = std::function<bool(uint64_t)>;

  /// Cone struct with SubnetID and mapping from cone subnet to original.
  struct Cone {
    Cone() = default;
    Cone(const SubnetID subnetID, const EntryMap &coneEntryToOrig):
      subnetID(subnetID), coneEntryToOrig(coneEntryToOrig) {};

    SubnetID subnetID;
    EntryMap coneEntryToOrig;
  };

  ConeBuilder() = delete;
  ConeBuilder(const ConeBuilder &other) = default;
  ConeBuilder &operator=(const ConeBuilder &other) = default;
  ~ConeBuilder() = default;

  /**
   * @brief ConeBuilder constructor.
   * @param subnet Subnet to find cones.
   */
  ConeBuilder(const Subnet *subnet);

  Cone getCone(const Cut &cut) const;
  Cone getMaxCone(const uint64_t entryIdx) const;

private:
  /**
   * @brief Finds and returns cone.
   * @param rootEntryIdx Root of cone.
   * @param isInEntry Function to check if passed entry is input for cone.
   */
  Cone getCone(uint64_t rootEntryIdx, const EntryCheckFunc &isInEntry) const;

private:
  const Subnet *subnet;
};

} // namespace eda::gate::optimizer2
