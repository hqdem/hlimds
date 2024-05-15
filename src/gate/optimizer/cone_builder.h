//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/cut_extractor.h"

#include <functional>
#include <stack>

namespace eda::gate::optimizer {

/// Builds cone as structure with Subnet and entries mapping.
class ConeBuilder {
public:
  using Subnet = model::Subnet;
  using Entry = Subnet::Entry;
  using SubnetID = model::SubnetID;
  using Link = Subnet::Link;
  using LinkList = Subnet::LinkList;
  using SubnetBuilder = model::SubnetBuilder;
  using Cut = optimizer::CutExtractor::Cut;

  using EntryVecMap = std::vector<size_t>;
  using EntryMapMap = std::unordered_map<size_t, size_t>;

  /**
   * @brief Cone struct with SubnetID and mapping from cone subnet to original.
   * Mapping contains all cells entries from the cone except root cell. Root
   * cell from the original subnet is referenced by PO from the cone subnet.
   */
  struct Cone {
    Cone() = default;
    Cone(const SubnetID subnetID, const EntryVecMap &coneEntryToOrig):
        subnetID(subnetID), coneEntryToOrig(coneEntryToOrig) {};

    SubnetID subnetID;
    EntryVecMap coneEntryToOrig;
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

  ConeBuilder(const SubnetBuilder *builder);

  Cone getCone(const Cut &cut) const;
  Cone getMaxCone(const size_t entryIdx) const;

private:
  /**
   * @brief Adds inner cells and primary output cells to cone and returns cone.
   */
  Cone getCone(const size_t rootEntryIdx,
               SubnetBuilder &builder,
               EntryMapMap &origEntryToCone,
               EntryVecMap &coneEntryToOrig) const;

  /**
   * @brief Adds primary inputs to cone limited by cut.
   */
  void addInsFromCut(const Cut &cut,
                     SubnetBuilder &builder,
                     EntryMapMap &origEntryToCone,
                     EntryVecMap &coneEntryToOrig) const;

  /**
   * @brief Adds primary inputs to maximum cone.
   */
  void addInsForMaxCone(const size_t rootEntryIdx,
                        SubnetBuilder &builder,
                        EntryMapMap &origEntryToCone,
                        EntryVecMap &coneEntryToOrig) const;

  /**
   * @brief Adds primary input to cone.
   */
  void addInput(const size_t origEntryIdx,
                const size_t rootEntryIdx,
                SubnetBuilder &builder,
                EntryMapMap &origEntryToCone,
                EntryVecMap &coneEntryToOrig) const;

  const Entry getEntry(const size_t entryID) const;

  const LinkList getLinks(const size_t entryID) const;

private:
  const Subnet *subnet;
  const SubnetBuilder *builder;
};

} // namespace eda::gate::optimizer
