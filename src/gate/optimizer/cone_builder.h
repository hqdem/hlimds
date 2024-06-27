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
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
  using InOutMapping = SubnetBuilder::InOutMapping;
  using Cut = optimizer::CutExtractor::Cut;

  using EntryVec = std::vector<size_t>;
  using EntryMap = std::unordered_map<size_t, size_t>;

  /**
   * @brief Cone struct with SubnetID, full mapping from cone subnet to original
   * and mapping from cone PI's and PO to original entries.
   * The full mapping contains all entries from the cone except the root.
   * The root entry from the original subnet is referenced by PO from the cone
   * subnet.
   */
  struct Cone {
  public:
    Cone(
        const SubnetID subnetID,
        const EntryVec &coneEntryToOrig):
      subnetID(subnetID), coneEntryToOrig(coneEntryToOrig),
      iomapping(getInOutMapping(subnetID, coneEntryToOrig)) {};

  private:
    inline InOutMapping getInOutMapping(const SubnetID subnetID,
                                        const EntryVec &coneEntryToOrig) {
      const auto &subnet = Subnet::get(subnetID);
      EntryVec inputs(subnet.getInNum()), outputs(subnet.getOutNum());
      for (size_t i = 0; i < subnet.getInNum(); ++i) {
        const auto inputID = subnet.getInIdx(i);
        inputs[i] = coneEntryToOrig[inputID];
      }
      for (size_t i = 0; i < subnet.getOutNum(); ++i) {
        const auto outputID = subnet.getOutIdx(i);
        outputs[i] = coneEntryToOrig[outputID];
      }
      return InOutMapping{inputs, outputs};
    }

  public:
    // Cone subnet.
    const SubnetID subnetID;
    // Full mapping from the cone entries to the original subnet entries.
    const EntryVec coneEntryToOrig;
    // Mapping from the cone's PI and PO to the original subnet entries.
    const InOutMapping iomapping;
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

  template <typename EntryIdxs>
  Cone getCone(const size_t rootEntryIdx, const EntryIdxs &cut) const;

  Cone getCone(const Cut &cut) const;
  Cone getMaxCone(const size_t rootEntryIdx) const;

private:
  /**
   * @brief Adds inner cells and primary output cells to cone and returns cone.
   */
  Cone getCone(const size_t rootEntryIdx,
               SubnetBuilder &builder,
               EntryMap &origEntryToCone,
               EntryVec &coneEntryToOrig) const;

  /**
   * @brief Adds primary inputs to cone limited by unordered or ordered cut.
   */
  template <typename EntryIdxs>
  void addInsFromCut(const size_t rootEntryIdx,
                     const EntryIdxs &cut,
                     SubnetBuilder &builder,
                     EntryMap &origEntryToCone,
                     EntryVec &coneEntryToOrig) const;

  /**
   * @brief Adds primary inputs to maximum cone.
   */
  void addInsForMaxCone(const size_t rootEntryIdx,
                        SubnetBuilder &builder,
                        EntryMap &origEntryToCone,
                        EntryVec &coneEntryToOrig) const;

  /**
   * @brief Adds primary input to cone.
   */
  void addInput(const size_t origEntryIdx,
                const size_t rootEntryIdx,
                SubnetBuilder &builder,
                EntryMap &origEntryToCone,
                EntryVec &coneEntryToOrig) const;

  const Entry getEntry(const size_t entryID) const;

  const LinkList getLinks(const size_t entryID) const;

private:
  const Subnet *subnet;
  const SubnetBuilder *builder;
};

template <typename EntryIdxs>
ConeBuilder::Cone ConeBuilder::getCone(const size_t rootEntryIdx,
                                       const EntryIdxs &cut) const {
  SubnetBuilder builder;
  EntryMap origEntryToCone;
  EntryVec coneEntryToOrig;

  addInsFromCut<EntryIdxs>(
      rootEntryIdx, cut, builder, origEntryToCone, coneEntryToOrig);

  return getCone(rootEntryIdx, builder, origEntryToCone, coneEntryToOrig);
}

template <typename EntryIdxs>
void ConeBuilder::addInsFromCut(const size_t rootEntryIdx,
                                const EntryIdxs &cut,
                                SubnetBuilder &builder,
                                EntryMap &origEntryToCone,
                                EntryVec &coneEntryToOrig) const {

  for (const auto &inEntryIdx : cut) {
    addInput(inEntryIdx, rootEntryIdx, builder, origEntryToCone,
             coneEntryToOrig);
  }
}

} // namespace eda::gate::optimizer
