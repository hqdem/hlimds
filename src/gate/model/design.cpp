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

static NetID getCellTypeNetID(const CellTypeID typeID) {
  const auto &type = CellType::get(typeID);
  if (type.isNet()) {
    return type.getNetID();
  } 
  return OBJ_NULL_ID;
}

DesignBuilder::DesignBuilder(const std::string &name, const NetID netID):
    name(name), typeID(OBJ_NULL_ID), netID(netID) {
  initialize(netID);
}

DesignBuilder::DesignBuilder(const NetID netID):
    DesignBuilder(DefaultName, netID) {}

DesignBuilder::DesignBuilder(const std::string &name, const SubnetID subnetID):
      name(name), typeID(OBJ_NULL_ID), netID(OBJ_NULL_ID) {
  initialize(subnetID);
}

DesignBuilder::DesignBuilder(const SubnetID subnetID):
    DesignBuilder(DefaultName, subnetID) {}

DesignBuilder::DesignBuilder(const std::string &name, const CellTypeID typeID):
    name(name), typeID(typeID), netID(getCellTypeNetID(typeID)) {
  const auto &type = CellType::get(typeID);
  assert(type.hasImpl());

  if (type.isNet()) {
    initialize(type.getNetID());
  } else {
    initialize(type.getSubnetID());
  }
}

DesignBuilder::DesignBuilder(const CellTypeID typeID):
    DesignBuilder(DefaultName, typeID) {}

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
                    const std::vector<uint32_t> &newInputs,
                    const std::vector<uint32_t> &newOutputs,
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
                                const std::vector<uint32_t> &newInputs,
                                const std::vector<uint32_t> &newOutputs) {
  auto &subnets = result.subnets;
  for (size_t i = 0; i < subnets.size(); ++i) {
    auto &mapping = subnets[i].mapping;
    replace(oldCellID, newCellID, newInputs, newOutputs, mapping.inputs);
    replace(oldCellID, newCellID, newInputs, newOutputs, mapping.outputs);
  }
}

//===----------------------------------------------------------------------===//
// Private Methods
//===----------------------------------------------------------------------===//

static void identifyClockAndResetDomains(
    const Net &net,
    std::vector<ClockDomain> &clockDomains,
    std::vector<ResetDomain> &resetDomains) {

  std::unordered_map<CellID, size_t> clocks;
  std::unordered_map<CellID, size_t> resets;

  const auto cells = net.getFlipFlops();
  for (auto i = cells.begin(); i != cells.end(); ++i) {
    const auto &cell = Cell::get(*i);
    const auto &type = cell.getType();

    if (type.isDff() || type.isSDff() || type.isADff() || type.isDffRs()) {
      const auto &source = cell.getLink(CellPin::DFF_IN_CLK);
      const auto sourceID = source.getCellID();

      size_t index = clocks.size();
      if (const auto found = clocks.find(sourceID); found != clocks.end()) {
        index = found->second;
      } else {
        clocks.emplace(sourceID, clocks.size());
        clockDomains.emplace_back(sourceID);
      }

      clockDomains[index].flipFlops.push_back(*i);
    }

    // TODO: Fill the reset domains.
  }
}

void DesignBuilder::initialize(const NetID netID) {
  assert(netID != OBJ_NULL_ID);
  const auto &net = Net::get(netID);

  nIn = net.getInNum();
  nOut = net.getOutNum();

  identifyClockAndResetDomains(net, clockDomains, resetDomains);

  // Generate the soft block implementations.
  synthesizer::synthSoftBlocks(netID);
  // Decompose the net into subnets.
  NetDecomposer::get().decompose(netID, result);

  SubnetToSubnetSet adjList;
  SubnetToArcDescs arcDescs;
  setAdjList(result, adjList, arcDescs);
  setEntries(result, adjList, arcDescs);
}

void DesignBuilder::initialize(const SubnetID subnetID) {
  assert(subnetID != OBJ_NULL_ID);
  const auto &subnet = Subnet::get(subnetID);

  nIn = subnet.getInNum();
  nOut = subnet.getOutNum();

  NetDecomposer::get().decompose(subnetID, result);
  SubnetToSubnetSet adjList;
  SubnetToArcDescs arcDescs;
  setAdjList(result, adjList, arcDescs);
  setEntries(result, adjList, arcDescs);
}

DesignBuilder::SubnetToFFSet DesignBuilder::findFlipFlopPIs(
    const NetDecomposer::Result &result) const {
  const auto &subnets = result.subnets;

  SubnetToFFSet flipFlopPIs(subnets.size());
  for (size_t i = 0; i < subnets.size(); ++i) {
    const auto &subnet = Subnet::get(subnets[i].subnetID);
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
    const NetDecomposer::Result &result,
    const SubnetToFFSet &flipFlopPIs,
    SubnetToArcDescs &arcDesc) const {
  const auto &subnets = result.subnets;

  SubnetToSubnetSet adjList(subnets.size());
  arcDesc.resize(subnets.size());
  for (size_t i = 0; i < subnets.size(); ++i) {
    const auto &subnet = Subnet::get(subnets[i].subnetID);
    for (uint16_t outN = 0; outN < subnet.getOutNum(); ++outN) {
      const EntryID outEntryID = subnet.getOutIdx(outN);
      const auto &outCell = subnet.getCell(outEntryID);
      const auto PILinking = getPIConnectionEntry(i);
      const auto POLinking = getPOConnectionEntry(i);
      if (PILinking.first) {
        adjList[i].insert(PISubnetEntryIdx);
        arcDesc[i][PISubnetEntryIdx] =
            subnets[i].entryToDesc.find(PILinking.second)->second;
      }
      if (POLinking.first) {
        adjList[i].insert(POSubnetEntryIdx);
        arcDesc[i][POSubnetEntryIdx] =
            subnets[i].entryToDesc.find(POLinking.second)->second;
      }
      if (!outCell.isFlipFlop()) {
        continue;
      }
      const auto flipFlopID = outCell.flipFlopID;
      for (size_t j = 0; j < flipFlopPIs.size(); ++j) {
        if (flipFlopPIs[j].find(flipFlopID) != flipFlopPIs[j].end()) {
          adjList[j].insert(i);
          arcDesc[j][i] = subnets[i].entryToDesc.find(outEntryID)->second;
        }
      }
    }
  }
  return adjList;
}

void DesignBuilder::setAdjList(
    const NetDecomposer::Result &result,
    SubnetToSubnetSet &adjList,
    SubnetToArcDescs &arcDesc) const {
  auto flipFlopPIs = findFlipFlopPIs(result);
  adjList = findArcs(result, flipFlopPIs, arcDesc);
}

} // namespace eda::gate::model
