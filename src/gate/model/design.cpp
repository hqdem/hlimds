//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/design.h"

#include <cassert>
#include <unordered_set>

namespace eda::gate::model {

using LinkSet = std::unordered_set<Link>;
using LinkMap = NetDecomposer::LinkMap;

std::tuple<EntryID, EntryID, EntryID> DesignBuilder::getCellNum(
    const size_t i,
    const bool withBufs) const {
  EntryID nI, nO, nC, nB;
  const auto &entry = getEntry(i);
  if (entry.subnetID != OBJ_NULL_ID) {
    const auto &subnet = Subnet::get(entry.subnetID);
    nI = subnet.getInNum();
    nO = subnet.getOutNum();
    nC = subnet.getCellNum();
    nB = !withBufs ? subnet.getBufNum() : 0;
  } else {
    assert(entry.builder != nullptr);
    const auto &builder = entry.builder;
    nI = builder->getInNum();
    nO = builder->getOutNum();
    nC = builder->getCellNum();
    nB = !withBufs ? builder->getBufNum() : 0;
  }

  return std::make_tuple(nI, nO, nC - nI - nO - nB);
}

static void replace(const CellID oldCellID, const CellID newCellID,
                    const std::vector<uint16_t> &newInputs,
                    const std::vector<uint16_t> &newOutputs,
                    LinkMap &linkMap) {
  assert(oldCellID != newCellID);

  LinkSet removeSet;
  LinkMap insertMap;

  for (const auto &[oldLink, idx] : linkMap) {
    const auto &oldSource = oldLink.source;
    const auto &oldTarget = oldLink.target;

    const auto isOldSource = (oldSource.getCellID() == oldCellID);
    const auto isOldTarget = (oldTarget.getCellID() == oldCellID);

    if (isOldSource || isOldTarget) {
      LinkEnd newSource{newCellID, newOutputs[oldSource.getPort()]};
      LinkEnd newTarget{newCellID, newInputs[oldTarget.getPort()]};

      Link newLink{(isOldSource ? newSource : oldSource),
                   (isOldTarget ? newTarget : oldTarget)};

      removeSet.insert(oldLink);
      insertMap.insert({newLink, idx});
    }
  }

  for (const auto oldLink : removeSet) {
    linkMap.erase(oldLink);
  }

  linkMap.insert(insertMap.begin(), insertMap.end());
}

void DesignBuilder::replaceCell(const CellID oldCellID, const CellID newCellID,
                                const std::vector<uint16_t> &newInputs,
                                const std::vector<uint16_t> &newOutputs) {
  for (size_t i = 0; i < mapping.size(); ++i) {
    replace(oldCellID, newCellID, newInputs, newOutputs, mapping[i].inputs);
    replace(oldCellID, newCellID, newInputs, newOutputs, mapping[i].outputs);
  }
}

DesignBuilder::SubnetToFFSet DesignBuilder::findFlipFlopPIs(
    const std::vector<SubnetID> &subnetIDs) const {

  SubnetToFFSet flipFlopPIs(subnetIDs.size());
  for (size_t i = 0; i < subnetIDs.size(); ++i) {
    const auto &subnet = Subnet::get(subnetIDs[i]);
    for (EntryID inN = 0; inN < subnet.getInNum(); ++inN) {
      const EntryID inEntryID = subnet.getInIdx(inN);
      const auto &inCell = subnet.getCell(inEntryID);
      if (inCell.isFlipFlop()) {
        flipFlopPIs[i].insert(inCell.flipFlopID);
      }
    }
  }
  return flipFlopPIs;
}

DesignBuilder::SubnetToSubnetSet DesignBuilder::findArcs(
    const std::vector<SubnetID> &subnetIDs,
    const std::vector<NetDecomposer::EntryToDesc> &ioEntryDesc,
    const SubnetToFFSet &flipFlopPIs,
    SubnetToArcDescs &arcDesc) const {

  SubnetToSubnetSet adjList(subnetIDs.size());
  arcDesc.resize(subnetIDs.size());
  for (size_t i = 0; i < subnetIDs.size(); ++i) {
    const auto &subnet = Subnet::get(subnetIDs[i]);
    for (uint16_t outN = 0; outN < subnet.getOutNum(); ++outN) {
      const EntryID outEntryID = subnet.getOutIdx(outN);
      const auto &outCell = subnet.getCell(outEntryID);
      const auto PILinking = getPIConnectionEntry(i);
      const auto POLinking = getPOConnectionEntry(i);
      if (PILinking.first) {
        adjList[i].insert(PISubnetEntryIdx);
        arcDesc[i][PISubnetEntryIdx] =
            ioEntryDesc[i].find(PILinking.second)->second;
      }
      if (POLinking.first) {
        adjList[i].insert(POSubnetEntryIdx);
        arcDesc[i][POSubnetEntryIdx] =
            ioEntryDesc[i].find(POLinking.second)->second;
      }
      if (!outCell.isFlipFlop()) {
        continue;
      }
      const auto flipFlopID = outCell.flipFlopID;
      for (size_t j = 0; j < flipFlopPIs.size(); ++j) {
        if (flipFlopPIs[j].find(flipFlopID) != flipFlopPIs[j].end()) {
          adjList[j].insert(i);
          arcDesc[j][i] = ioEntryDesc[i].find(outEntryID)->second;
        }
      }
    }
  }
  return adjList;
}

void DesignBuilder::setAdjList(
    const std::vector<SubnetID> &subnetIDs,
    const std::vector<NetDecomposer::EntryToDesc> &ioEntryDesc,
    SubnetToSubnetSet &adjList,
    SubnetToArcDescs &arcDesc) const {
  auto flipFlopPIs = findFlipFlopPIs(subnetIDs);
  adjList = findArcs(subnetIDs, ioEntryDesc, flipFlopPIs, arcDesc);
}

} // namespace eda::gate::model
