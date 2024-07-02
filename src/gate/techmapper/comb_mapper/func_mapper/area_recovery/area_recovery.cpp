//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/techmapper/comb_mapper/func_mapper/area_recovery/area_recovery.h"

namespace eda::gate::techmapper {

float AreaRecovery::getMinAreaAndCell(
        SubnetID &cellTechLib, Cut &cut, const SCLibrary &cellDB) const {
  SubnetBuilder builder(subnetID); // FIXME:
  SubnetView window(builder, cut);
  const auto truthTable = window.evaluateTruthTable();
  std::vector<SubnetID> cellList = cellDB.getSubnetID(truthTable);
  if (cellList.empty()) {
    return 0;
  }

  float minCellArea = 0;
  bool initedMinArea = false;
  for (size_t cellIndex = 0; cellIndex < cellList.size(); cellIndex++) {
    const auto &attr = cellDB.getCellAttrs(cellList[cellIndex]);
    if (attr.area < minCellArea || !initedMinArea) {
      minCellArea = attr.area;
      initedMinArea = true;
      cellTechLib = cellList[cellIndex];
    }
  }

  return minCellArea;
}

double
AreaRecovery::calcAreaFlow(Cut &cut, std::vector<double> &representAreaFlow,
                           model::Array<Subnet::Entry> &entries,
                          float minArea) {
  double aF = minArea;

  SubnetBuilder builder(subnetID); // FIXME:
  SubnetView window(builder, cut);

  // FIXME: Traverse the window and calculates the area flow.

  /*
  const SubnetID &coneSubnetID = cone.subnetID;
  Subnet &coneSubnet = Subnet::get(coneSubnetID);
  eda::gate::model::Array<Subnet::Entry> coneEntries = coneSubnet.getEntries();

  for (uint64_t coneEntryIndex = cut.entryIdxs.size();
       coneEntryIndex < std::size(coneEntries); coneEntryIndex++) {
    const uint64_t &coneEntryIndexInOrig = cone.coneEntryToOrig[coneEntryIndex];
    if (coneEntryIndexInOrig == 0) {
      continue;
    }
    Subnet::Cell &coneCellInOrig = entries[coneEntryIndexInOrig].cell;
    for (Subnet::Link &inLink : coneCellInOrig.link) {
      if (cut.entryIdxs.find(inLink.idx) != cut.entryIdxs.end()) {
        Subnet::Cell &inCell = entries[inLink.idx].cell;
        aF += representAreaFlow[inLink.idx] / inCell.refcount;
      }
    }
  }
  */
  return aF;
}

double
AreaRecovery::calcDepth(std::vector<double> &depth,
                          eda::gate::model::Array<Subnet::Entry> &entries,
                          uint64_t entryIndex, Cut &cut) {
  if (depth[entryIndex] == 0) {
    double maxDepth = 0.0;
    Subnet::Cell &cell = entries[entryIndex].cell;
    for (Subnet::Link &inLink : cell.link) {
      if (depth[inLink.idx] > maxDepth) {
        maxDepth = depth[inLink.idx];
      }
    }
    depth[entryIndex] = 1 + maxDepth;
  }

  double maxDepthCone = 0.0;
  for (uint64_t inCellIdx : cut.entryIdxs) {
    if (depth[inCellIdx] > maxDepthCone) {
      maxDepthCone = depth[inCellIdx];
    }
  }
  maxDepthCone += 1;

  return maxDepthCone;
}

void AreaRecovery::map(
       const SubnetID subnetID, const SCLibrary &cellDB,
       const SDC &sdc, Mapping &mapping) {
  this->subnetID = subnetID;
  cutExtractor = new optimizer::CutExtractor(&model::Subnet::get(subnetID), 6);
  Subnet &subnet = Subnet::get(this->subnetID);
  model::Array<Subnet::Entry> entries = subnet.getEntries();

  std::vector<double> representAreaFlow(std::size(entries), 0.0);
  std::vector<double> representDepth(std::size(entries), 0.0);
  std::vector<double> depth(std::size(entries), 0.0);

  for (uint64_t entryIndex = 0; entryIndex < std::size(entries); entryIndex++) {
    Subnet::Cell &cell = entries[entryIndex].cell;
    if (cell.isIn() || cell.isOut() || cell.isOne() || cell.isZero()) {
      addNotAnAndToTheMap(entryIndex, cell, mapping);
      continue;
    }

    MappingItem mappingItem;

    std::vector<Cut> cutsList = cutExtractor->getCuts(entryIndex);
    for (Cut &cut : cutsList) {
      if (cut.entryIdxs.count(entryIndex) == 1) {
        continue;
      }

      SubnetID cellTechLib;
      float minCellArea = getMinAreaAndCell(cellTechLib, cut, cellDB);
      if (minCellArea == 0) {
        continue;
      }
      double aF = calcAreaFlow(cut, representAreaFlow, entries, minCellArea);
      double maxDepth = calcDepth(depth, entries, entryIndex, cut);

      if (representAreaFlow[entryIndex] == 0 ||
          aF < representAreaFlow[entryIndex] ||
          (aF == representAreaFlow[entryIndex] &&
           maxDepth < representDepth[entryIndex])) {

        representAreaFlow[entryIndex] = aF;
        representDepth[entryIndex] = maxDepth;

        mappingItem.setSubnetID(cellTechLib);
        mappingItem.inputs.clear();
        for (const size_t &in : cut.entryIdxs) {
          mappingItem.inputs.push_back(in);
        }
      }
    }
    mapping[entryIndex] = mappingItem;
  }
}

} // namespace eda::gate::techmapper
