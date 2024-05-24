//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/techmapper/comb_mapper/cut_based/area_recovery/area_recovery.h"

namespace eda::gate::techmapper {

float AreaRecovery::getMinAreaAndCell(SubnetID &cellTechLib, Cut &cut) {
  ConeBuilder coneBuilder(&Subnet::get(this->subnetID));
  SubnetID coneSubnetID = coneBuilder.getCone(cut).subnetID;
  const auto truthTable =
      eda::gate::model::evaluate(Subnet::get(coneSubnetID))[0];
  std::vector<SubnetID> cellList = cellDB->getSubnetIDsByTT(truthTable);
  if (cellList.size() == 0) {
    return 0;
  }

  float minCellArea = 0;
  bool initedMinArea = false;
  for (size_t cellIndex = 0; cellIndex < cellList.size(); cellIndex++) {
    Subnetattr attr = cellDB->getSubnetAttrBySubnetID(cellList[cellIndex]);
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
                             eda::gate::model::Array<Subnet::Entry> &entries,
                             float minArea) {
  double aF = minArea;

  ConeBuilder coneBuilder(&Subnet::get(this->subnetID));
  Cone cone = coneBuilder.getCone(cut);
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

void AreaRecovery::findBest() {
  Subnet &subnet = Subnet::get(this->subnetID);
  eda::gate::model::Array<Subnet::Entry> entries = subnet.getEntries();

  std::vector<double> representAreaFlow(std::size(entries), 0.0);
  std::vector<double> representDepth(std::size(entries), 0.0);
  std::vector<double> depth(std::size(entries), 0.0);

  for (uint64_t entryIndex = 0; entryIndex < std::size(entries); entryIndex++) {
    Subnet::Cell &cell = entries[entryIndex].cell;
    if (cell.isIn() || cell.isOut() || cell.isOne() || cell.isZero()) {
      addNotAnAndToTheMap(entryIndex, cell);
      continue;
    }

    BestReplacement bestReplacement{};

    std::vector<Cut> cutsList = cutExtractor->getCuts(entryIndex);
    for (Cut &cut : cutsList) {
      if (cut.entryIdxs.count(entryIndex) == 1) {
        continue;
      }

      SubnetID cellTechLib;
      float minCellArea = getMinAreaAndCell(cellTechLib, cut);
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

        bestReplacement.setSubnetID(cellTechLib);
        bestReplacement.inputs.clear();
        for (const size_t &in : cut.entryIdxs) {
          bestReplacement.inputs.push_back(in);
        }
      }
    }
    (*bestReplacementMap)[entryIndex] = bestReplacement;
  }
}

} // namespace eda::gate::techmapper
