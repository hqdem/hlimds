//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/analyzer/simulation_estimator.h"
#include "gate/model/utils/subnet_truth_table.h"
#include "gate/techmapper/comb_mapper/cut_based/power_map/power_map.h"

namespace eda::gate::techmapper {

using Subnet = eda::gate::model::Subnet;
using SwitchActivity = eda::gate::analyzer::SwitchActivity;
using Entry = Subnet::Entry;
using ArrayEntry = eda::gate::model::Array<Entry>;
using Cut = eda::gate::optimizer::CutExtractor::Cut;

double PowerMap::getSwitching(const Cut &cut) {
  auto entryIndex = cut.rootEntryIdx;
  return switchActivity.getActivities()[entryIndex];
  // return (double) switchActivity.getSwitchesOn()[entryIndex] + 
  //   switchActivity.getSwitchesOff()[entryIndex];
}

double PowerMap::switchFlow(const EntryIndex entryIndex,
                            const Cut &cut) {
  double sf = getSwitching(cut);
  
  for (const auto &leafIdx : cut.entryIdxs) {
    const auto &leaf = (*entries)[leafIdx].cell;
    if(leaf.isIn()) {
      metrics[leafIdx].sf = switchActivity.getActivities()[leafIdx];
    }
    sf += metrics[leafIdx].sf / leaf.refcount;
  }
  return sf;
}

double PowerMap::getArea(const Cut &cut) {
  auto entryIndex = cut.rootEntryIdx;
  const auto &currentCell = (*entries)[entryIndex].cell;
  if(currentCell.isIn())return 0;
  return 1 + double(cut.entryIdxs.size()) / 2;
}

double PowerMap::areaFlow(const EntryIndex entryIndex,
                          const Cut &cut) {
  double af = getArea(cut);

  for (const auto &leafIdx : cut.entryIdxs) {
    const auto &leaf = (*entries)[leafIdx].cell;
    if (leaf.isIn())
      continue;
    af += metrics[leafIdx].af / leaf.refcount;
  }
  return af;
}

uint32_t PowerMap::getLevel(const EntryIndex entryIdx) {
  return metrics[entryIdx].level;
}

uint32_t PowerMap::getLevel(const Cut &cut) {
  uint32_t levelMax = 0;
  for (const EntryIndex &leafIdx : cut.entryIdxs) {
    levelMax = std::max(levelMax, getLevel(leafIdx));
  }
  return levelMax + 1;
}

uint32_t PowerMap::getLevel(const std::vector<uint64_t> &entryIdxs) {
  uint32_t levelMax = 0;
  for (const EntryIndex &leafIdx : entryIdxs) {
    levelMax = std::max(levelMax, getLevel(leafIdx));
  }
  return levelMax + 1;
}

void PowerMap::ref(const EntryIndex &entryIndex) {
  for(const auto& leafIdx : (*bestReplacementMap)[entryIndex].inputs) {
    metrics[leafIdx].refCounter++;
  }
}
void PowerMap::deref(const EntryIndex &entryIndex) {
  for(const auto& leafIdx : (*bestReplacementMap)[entryIndex].inputs) {
    metrics[leafIdx].refCounter--;
  }
}

void PowerMap::saveBestReplacement(EntryIndex entryIndex,
                                  const Cut &cut,
                                  const SubnetID techSubnetId) {

  assert(techSubnetId != 0);
  assert(!cut.entryIdxs.empty());
  if(!(*bestReplacementMap)[entryIndex].inputs.empty()) {
    deref(entryIndex);
  }
  (*bestReplacementMap)[entryIndex].inputs.clear();
  for (const auto &in : cut.entryIdxs) {
    (*bestReplacementMap)[entryIndex].inputs.push_back(in);
  }
  ref(entryIndex);
  (*bestReplacementMap)[entryIndex].setSubnetID(techSubnetId);
}

double PowerMap::getCellArea(const Cut &cut, const SubnetID &techCellSubnetId) {
  return cellDB->getSubnetAttrBySubnetID(techCellSubnetId).area;
}

void PowerMap::findCutMinimizingDepth(const EntryIndex entryIndex) {
  SubnetID techSubnetId = 0;

  Cut bestCut = Cut();
  uint32_t cutBestLevel = UINT32_MAX;
  const CutsList &cutsList = cutExtractor->getCuts(entryIndex);

  for (const Cut &cut : cutsList) {
    if (cut.entryIdxs.count(entryIndex) == 1)
          continue;
    uint32_t curLevel = getLevel(cut);
    if (bestCut.entryIdxs.empty() || cutBestLevel > curLevel) {
      SubnetID tmpSubnetID = getBestTechCellSubnetId(cut);
      if (tmpSubnetID == 0)continue;
      techSubnetId = tmpSubnetID;
      cutBestLevel = curLevel;
      bestCut = cut;
    }
  }
  metrics[entryIndex].level = cutBestLevel; // setLevel(n, getLevel(cut))
  saveBestReplacement(entryIndex,bestCut,techSubnetId);
  assert(bestReplacementMap->at(entryIndex).getSubnetID() != 0);
}

void PowerMap::depthOrientedMap() {
  const auto &subnet =  Subnet::get(subnetID);
  for (uint64_t entryIndex = subnet.getInNum();
   entryIndex < entries->size() - subnet.getOutNum(); entryIndex++) {
    const auto &cell = (*entries)[entryIndex].cell;
    (*bestReplacementMap)[entryIndex].inputs.clear();
    if (cell.isAnd() || cell.isBuf()) {
      findCutMinimizingDepth(entryIndex);
    }
    else {
      addNotAnAndToTheMap(entryIndex, (*entries)[entryIndex].cell);
    }
  }
}

bool approxEqual(const double &a, const double &b, const double &eps = 0.01) {
  return fabs(a-b) < eps;
}

void PowerMap::computeRequiredTimes() {
   const Subnet &subnet = Subnet::get(subnetID);

  // get latest Primary Output arival time
  uint32_t timeMax = findLatestPoArivalTime();

  for (auto &metric : metrics)
    metric.requiredTime = UINT32_MAX;

  //initialize required times
  for (int32_t i = 0; i < subnet.getOutNum(); i++) {
    metrics[subnet.size() - i - 1].requiredTime = timeMax;
  }

  // propagate the required times for AIG nodes
  // in reverse topological order
  for (int32_t entryIdx = entries->size() - subnet.getOutNum() - 1; entryIdx >= 0; entryIdx--) {
    uint32_t timeReqNew = metrics[entryIdx].requiredTime - 1;
    // for each leaf in Representative Cut
    for (const auto &leafIdx : bestReplacementMap->at(entryIdx).inputs) {
      uint32_t timeReqOld = metrics[leafIdx].requiredTime;
      metrics[leafIdx].requiredTime = std::min(timeReqOld, timeReqNew);
    }
  }
}

//TODO
//options:
//  1) *each cut-defined techCell has a unit delay
//  2) delay for techCell is computed based on the longest internal path where an AND-gate has a unit delay 
//  3) get delay for techCell form liberty parser    
uint32_t PowerMap::findLatestPoArivalTime() {
  uint32_t latestPoArivalTime = 0;
  const Subnet &subnet = Subnet::get(subnetID);
  for(int i = 0; i < subnet.getOutNum(); i++) {
    latestPoArivalTime = std::max(
      getLevel(bestReplacementMap->at(subnet.size() - i - 1).inputs),
      latestPoArivalTime);
  }
      
  // std::cout << latestPoArivalTime << std::endl;
  return latestPoArivalTime;
};

double PowerMap::getCellPower(const Cut &cut,
                              const SubnetID &techCellSubnetId) {

  const std::vector<size_t> &riseActivities =
    switchActivity.getSwitchesOn();
  const std::vector<size_t> &fallActivities =
    switchActivity.getSwitchesOff();
  auto currentAttr = cellDB->getSubnetAttrBySubnetID(techCellSubnetId);
  float cellPower = 0;
  int i = 0;
  for (const auto &leaf : cut.entryIdxs) {
    cellPower += std::abs(currentAttr.pinsPower[i].rise_power) * (float)riseActivities[leaf];
    cellPower += std::abs(currentAttr.pinsPower[i].fall_power) * (float)fallActivities[leaf];
    i++;
  }
  return cellPower;
};

SubnetID PowerMap::getBestTechCellSubnetId(const Cut &cut) {
  SubnetID bestTechCellSubnetID;
  const auto techIdsList = getTechIdsList(cut);
  if (techIdsList.size() == 0) return 0;
  double localCellPower = MAXFLOAT;
  for (const SubnetID &techCellSubnetId : techIdsList) {
    double curLocalCellPower = getCellPower(cut,techCellSubnetId);
    if (curLocalCellPower < localCellPower) {
      localCellPower = curLocalCellPower;  
      bestTechCellSubnetID = techCellSubnetId;
    }
  }
  assert(bestTechCellSubnetID != 0);
  return bestTechCellSubnetID;
}

SubnetID PowerMap::getBestAreaTechCellSubnetId(const Cut &cut) {
  SubnetID bestTechCellSubnetID;
  const auto techIdsList = getTechIdsList(cut);
  if (techIdsList.size() == 0) return 0;
  double localCellArea = MAXFLOAT;
  for (const SubnetID &techCellSubnetId : techIdsList) {
    double curLocalCellArea = getCellArea(cut,techCellSubnetId);
    if (curLocalCellArea < localCellArea) {
      localCellArea = curLocalCellArea;  
      bestTechCellSubnetID = techCellSubnetId;
    }
  }
  assert(bestTechCellSubnetID != 0);
  return bestTechCellSubnetID;
}

void PowerMap::globalSwitchAreaRecovery() {
  const auto &subnet =  Subnet::get(subnetID);
  for (uint64_t entryIndex = subnet.getInNum();
   entryIndex < entries->size() - subnet.getOutNum(); entryIndex++) {
    const auto &cell = (*entries)[entryIndex].cell;

    if (cell.isAnd() || cell.isBuf()) {
      const CutsList &cutsList = cutExtractor->getCuts(entryIndex);

      Cut bestCut = Cut();
      SubnetID bestTechCellSubnetID = 0;

      for (const Cut &cut : cutsList) {
        if (cut.entryIdxs.count(entryIndex) == 1)
          continue;
        
        double curAF = areaFlow(entryIndex, cut);
        double curSF = switchFlow(entryIndex, cut);

        if ((curAF < metrics[entryIndex].af) ||
            (approxEqual(curAF, metrics[entryIndex].af) && curSF < metrics[entryIndex].sf)) {

          if(/*getLevel(cut) <= metrics[entryIndex].requiredTime*/ true ) {
            SubnetID tmpSubnetId = getBestTechCellSubnetId(cut);
            if(tmpSubnetId == 0 )continue;
            bestTechCellSubnetID = tmpSubnetId;
            // metrics[entryIndex].level = getLevel(cut);
            metrics[entryIndex].af = curAF;
            metrics[entryIndex].sf = curSF;
            bestCut = cut;
          }
        }
      }

      if(bestCut.entryIdxs.empty()){
        assert(!(*bestReplacementMap)[entryIndex].inputs.empty());
        assert((*bestReplacementMap)[entryIndex].getSubnetID() != 0);
        continue;
      }   
      saveBestReplacement(entryIndex,bestCut,bestTechCellSubnetID);
    }
    else {
      addNotAnAndToTheMap(entryIndex, cell);
    }
    entryIndex += cell.more;
  }
}

double PowerMap::exactAreaRef(const Cut &cut) {
  double localArea = getArea(cut);
  for(const auto &leafIdx : cut.entryIdxs) {
    metrics[leafIdx].refCounter--;
    if(metrics[leafIdx].refCounter == 0 && (*entries)[leafIdx].cell.isIn()) {

      Cut virtCut = Cut();
      virtCut.rootEntryIdx = leafIdx;
      for(const auto &idx : bestReplacementMap->at(leafIdx).inputs) {
        virtCut.entryIdxs.insert(idx);
      }
      localArea += exactAreaRef(virtCut);
    }
  }
  return localArea;
}

double PowerMap::exactAreaDeref(const Cut &cut) {
  double localArea = getArea(cut);
  for(const auto &leafIdx : cut.entryIdxs) {
    if(metrics[leafIdx].refCounter == 0 && (*entries)[leafIdx].cell.isIn()) {

      Cut virtCut = Cut();
      virtCut.rootEntryIdx = leafIdx;
      for(const auto &idx : bestReplacementMap->at(leafIdx).inputs) {
        virtCut.entryIdxs.insert(idx);
      }
      localArea += exactAreaDeref(virtCut);
    }
    metrics[leafIdx].refCounter++;
  }
  return localArea;
}

bool PowerMap::cutIsRepr(EntryIndex entryIndex,const Cut &cut) {
  if(cut.entryIdxs.size() != bestReplacementMap->at(entryIndex).inputs.size())return 0;
  for(const auto &leafIdx : bestReplacementMap->at(entryIndex).inputs) {
    if(cut.entryIdxs.count(leafIdx) == 0)return 0;
  }
  return 1;
}

double PowerMap::exactArea(EntryIndex entryIndex, const Cut &cut) {
  double localArea1 = 0;

  if(cutIsRepr(entryIndex, cut)) {
    localArea1 = exactAreaDeref(cut);
    exactAreaRef(cut);
  }else{
    exactAreaRef(cut);
    localArea1 = exactAreaDeref(cut);
  }
  return localArea1;
}

double PowerMap::exactSwitch(EntryIndex entryIndex, const Cut &cut) {
  return MAXFLOAT;
}


void PowerMap::localSwitchAreaRecovery() {
  const auto &subnet =  Subnet::get(subnetID);
  for (uint64_t entryIndex = subnet.getInNum();
   entryIndex < entries->size() - subnet.getOutNum(); entryIndex++) {
    const auto &cell = (*entries)[entryIndex].cell;

    if (cell.isAnd() || cell.isBuf()) {

      CutsList cutsList = cutExtractor->getCuts(entryIndex);
      //initial values
      double bestArea = MAXFLOAT;
      double bestSwitch = MAXFLOAT;

      Cut bestCut = Cut();
      SubnetID bestTechCellSubnetID = 0;

      for (const Cut &cut : cutsList) {
        if (cut.entryIdxs.count(entryIndex) == 1)
          continue;
        
        double curArea = exactArea(entryIndex, cut);
        double curSwitch = exactSwitch(entryIndex, cut);

        if ((curArea < bestArea) ||
            (approxEqual(curArea, bestArea) && curSwitch < bestSwitch)) {

          if(getLevel(cut) <= metrics[entryIndex].requiredTime) {
            SubnetID tmpSubnetId = getBestTechCellSubnetId(cut);
            if(tmpSubnetId == 0 )continue;
            bestTechCellSubnetID = tmpSubnetId;
            metrics[entryIndex].level = getLevel(cut);
            bestArea = curArea;
            bestSwitch = curSwitch;
            bestCut = cut;
          }
        }
      }
      if(bestCut.entryIdxs.empty()) {
        assert(!(*bestReplacementMap)[entryIndex].inputs.empty());
        continue;
      }
      saveBestReplacement(entryIndex,bestCut,bestTechCellSubnetID);
      assert((*bestReplacementMap)[entryIndex].getSubnetID() != 0);
    }
    else {
      addNotAnAndToTheMap(entryIndex, cell);
    }
    entryIndex += cell.more;
  }
}

PowerMap::PowerMap() {
  entries = nullptr;
  coneBuilder = nullptr;
}

void PowerMap::findBest() {

#ifdef UTOPIA_DEBUG
  std::cerr << "Start PowerMap::findBest" << std::endl;
  auto start = std::chrono::high_resolution_clock::now();
#endif // UTOPIA_DEBUG

  init();
  // depthOrientedMap();
  // computeRequiredTimes();
  globalSwitchAreaRecovery();
  // computeRequiredTimes();
  // localSwitchAreaRecovery();
  clear();

#ifdef UTOPIA_DEBUG
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> FindBestTime = end - start;
  std::cerr << "PowerMap::findBest was running " << FindBestTime.count() << " seconds.\n";
#endif // UTOPIA_DEBUG
}

std::vector<SubnetID> PowerMap::getTechIdsList(const Cut cut) {
  SubnetID coneSubnetID = coneBuilder->getCone(cut).subnetID;
  std::vector<SubnetID> cellList;
  for(const auto &truthTable : eda::gate::model::evaluate(Subnet::get(coneSubnetID))){  
    for(const auto &id : cellDB->getSubnetIDsByTT(truthTable)){
      cellList.push_back(id);
    }
  }
  // assert(cellList.size() != 0);
  return cellList;
}

void PowerMap::init() {
  const Subnet &subnet = Subnet::get(this->subnetID);
  entries = new ArrayEntry(subnet.getEntries());
  metrics.resize(entries->size());
  coneBuilder = new eda::gate::optimizer::ConeBuilder(&subnet);

  // get switching activity of subnet
  eda::gate::analyzer::SimulationEstimator simulationEstimator(256);
  switchActivity = simulationEstimator.estimate(subnet);

  // save inputs in bestReplacementMap
  for (uint64_t entryIndex = 0; entryIndex < subnet.getInNum(); entryIndex++) {
    addInputToTheMap(entryIndex);
  }
  // save outputs in bestReplacementMap
  for (uint64_t entryIndex = subnet.size() - subnet.getOutNum();
   entryIndex < subnet.size(); entryIndex++) {
    const auto &cell = (*entries)[entryIndex].cell;
    addOutToTheMap(entryIndex,cell);
    metrics[entryIndex].level = 0; 
  }
}

void PowerMap::clear() {
  metrics.clear();
  delete entries;
  delete coneBuilder;
  coneBuilder = nullptr;
  entries = nullptr;
}
} //namespace eda::gate::techmapper
