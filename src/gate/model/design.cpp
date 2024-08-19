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

std::tuple<size_t, size_t, size_t> DesignBuilder::getCellNum(
    const size_t i,
    const bool withBufs) const {
  size_t nI, nO, nC, nB;
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

DesignBuilder::SubnetToSubnetSet DesignBuilder::findFlipFlopPOs(
    const std::vector<SubnetID> &subnetIDs) const {

  SubnetToSubnetSet flipFlopPOs(subnetIDs.size());
  // Find flip-flops POs.
  for (size_t i = 0; i < subnetIDs.size(); ++i) {
    const auto &subnet = Subnet::get(subnetIDs[i]);
    for (size_t outN = 0; outN < subnet.getOutNum(); ++outN) {
      const size_t outEntryID = subnet.getOutIdx(outN);
      const auto &outCell = subnet.getCell(outEntryID);
      if (outCell.isFlipFlop()) {
        flipFlopPOs[i].insert(outCell.flipFlopID);
      }
    }
  }
  return flipFlopPOs;
}

DesignBuilder::SubnetToSubnetSet DesignBuilder::findArcs(
    const std::vector<SubnetID> &subnetIDs,
    const SubnetToSubnetSet &flipFlopPOs) const {

  SubnetToSubnetSet adjList(subnetIDs.size());
  for (size_t i = 0; i < subnetIDs.size(); ++i) {
    const auto &subnet = Subnet::get(subnetIDs[i]);
    for (size_t inN = 0; inN < subnet.getInNum(); ++inN) {
      const auto &inCell = subnet.getCell(inN);
      if (!inCell.isFlipFlop()) {
        continue;
      }
      const auto flipFlopID = inCell.flipFlopID;
      for (size_t j = 0; j < flipFlopPOs.size(); ++j) {
        if (flipFlopPOs[j].find(flipFlopID) != flipFlopPOs[j].end()) {
          adjList[j].insert(i);
        }
      }
    }
  }
  return adjList;
}

DesignBuilder::SubnetToSubnetSet DesignBuilder::getAdjList(
    const std::vector<SubnetID> &subnetIDs) const {
  auto flipFlopPOs = findFlipFlopPOs(subnetIDs);
  return findArcs(subnetIDs, flipFlopPOs);
}

std::ostream &operator <<(std::ostream &out, const DesignBuilder &builder) {
  out << "digraph " << builder.getName() << " {\n";

  const size_t nSubnet = builder.getSubnetNum();
  std::vector<std::vector<size_t>> subnetLinks(nSubnet);
  for (size_t i = 0; i < nSubnet; ++i) {
    out << "  snet_" << i << ";\n";
    const auto &outArcs = builder.getOutArcs(i);
    for (const auto &outArc : outArcs) {
      subnetLinks[outArc].push_back(i);
    }
  }
  for (size_t i = 0; i < nSubnet; ++i) {
    for (const auto &link : subnetLinks[i]) {
      out << "  snet_" << link << " -> " <<  "snet_" << i << ";\n";
    }
  }

  out << "}\n";
  return out;
}

} // namespace eda::gate::model
