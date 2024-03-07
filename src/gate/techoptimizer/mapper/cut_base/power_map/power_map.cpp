//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/analyzer/simulation_estimator.h"
#include "gate/model2/utils/subnet_truth_table.h"
#include "gate/techoptimizer/mapper/cut_base/power_map/power_map.h"

namespace eda::gate::tech_optimizer {

  double PowerMap::switchFlow(const EntryIndex entryIndex,
                              const Cut &cut,
                              const std::vector<double> &cellActivities) {
    double sf = cellActivities[entryIndex];
    const auto &currentCell = (*entries)[entryIndex].cell; 
    if (!currentCell.isIn()) {
      for (const auto &leafIdx : cut.entryIdxs) {
        const auto &leaf = (*entries)[leafIdx].cell;
        if(leaf.isIn()) {
          computedSF.at(leafIdx) = cellActivities[leafIdx];
        } 
        sf += computedSF[leafIdx]/(leaf.refcount);
      }
    }
    computedSF[entryIndex] = sf;
    return sf;
  }
  
  double PowerMap::areaFlow(const EntryIndex entryIndex,
                            const Cut &cut) {
    double af = 0;
    const auto &currentCell  = (*entries)[entryIndex].cell;
    if (!currentCell.isIn()) {
      af = 1;
      for (const auto &leafIdx : cut.entryIdxs) { 
        const auto &leaf = (*entries)[leafIdx].cell;
        if(leaf.isIn())continue;
        af += computedAF[leafIdx]/(leaf.refcount);
      }
    }
    computedAF[entryIndex] = af;
    return af;
  }

  int64_t PowerMap::getLevel(const EntryIndex entryIdx) {
    return computedLevel[entryIdx];
  }

  int64_t PowerMap::getLevel(const Cut &cut) {
    int64_t levelMax = INT64_MIN;
    for(const EntryIndex &leafIdx: cut.entryIdxs) {
      levelMax = std::max(levelMax,getLevel(leafIdx));
    }
    return levelMax + 1;
  }

  BestReplacement PowerMap::findCutMinimizingDepth(const EntryIndex entryIndex) {
    
    SubnetID techSubnetId;

    const Cut *cutBest = nullptr;
    int64_t cutBestLevel = INT64_MAX;
    const CutsList &cutsList = cutExtractor->getCuts(entryIndex);

    for(const Cut &cut : cutsList) {

      int64_t curLevel = getLevel(cut);
      if (cutBest == nullptr || cutBestLevel > curLevel) {
        const auto techIdsList = getTechIdsList(cut);
        if(techIdsList.size() == 0)continue;
        techSubnetId = techIdsList[0];
        cutBestLevel = curLevel;
        cutBest = &cut;
      }
    }

    BestReplacement bestRepl;
    computedLevel[entryIndex] = cutBestLevel; // setLevel(n, getLevel(cut))
    bestRepl.entryIDxs =cutBest->entryIdxs;
    bestRepl.subnetID = techSubnetId;
    return bestRepl;
  }

  void PowerMap::traditionalMapDepthOriented() {
    for (uint64_t entryIndex = 0; entryIndex < entries->size(); entryIndex++) {
      if (!(*entries)[entryIndex].cell.isAnd()) {
        addNotAnAndToTheMap(entryIndex, (*entries)[entryIndex].cell);
        computedLevel[entryIndex] = 0;
        continue;
      }
      (*bestReplacementMap)[entryIndex] = findCutMinimizingDepth(entryIndex);    
    }  
  }

  bool aproxEqual(const double &a, const double &b, const double &eps = 0.0001) {
    return fabs(a-b) < eps;
  }

  uint32_t getDepth(const EntryIndex idx, const Cut &cut, const ArrayEntry &entries) {
    uint32_t depth = 1;
    if (cut.entryIdxs.find(idx) != cut.entryIdxs.end()) return 1;
    
    for (uint32_t i = 0; i < entries[idx].cell.arity; i++ ) {
      depth = std::max(depth, 1 + getDepth(entries[idx].cell.link[i].idx, cut, entries));
    }
    return depth;
  }

  void PowerMap::computeRequiredTimes() {
    // get latest Primary Output arival time
    const Subnet &subnet =Subnet::get(subnetID); 
    uint32_t timeMax = subnet.getPathLength().second;

    for(auto& reqTime : requiredTimes)reqTime = UINT32_MAX;

    for(int32_t i = 0; i < subnet.getOutNum();i++) {
      requiredTimes[subnet.getOut(i).idx] = timeMax;
    }
    //propagate the required times 
    //in reverse topological order
    for(int32_t entryIdx = entries->size()-1; entryIdx >= 0; entryIdx--) {
      uint32_t timeReqNew = requiredTimes[entryIdx] - 1;
      // for each leaf in Representative Cut
      for(const auto &leafIdx : bestReplacementMap->at(entryIdx).entryIDxs) {
        uint32_t timeReqOld = requiredTimes[leafIdx];
        requiredTimes[leafIdx] = std::min(timeReqOld,timeReqNew);
      }
    }
  }

  void PowerMap::globalSwitchAreaRecovery(const std::vector<double> &cellActivities) {
    
    for (uint64_t entryIndex = 0; entryIndex < entries->size();entryIndex++) {
      const auto &cell = (*entries)[entryIndex].cell;

      if(!cell.isAnd()) {
        addNotAnAndToTheMap(entryIndex,cell);
      } else {  
        CutsList cutsList = cutExtractor->getCuts(entryIndex);
        double bestAF = MAXFLOAT;
        double bestSF = MAXFLOAT;
        Cut bestCut = Cut();
        SubnetID bestTechCellSubnetID = 0;

        for(const Cut &cut : cutsList) {
          if(cut.entryIdxs.size() == 1)continue;
          double curAF = areaFlow(entryIndex,cut);
          double curSF = switchFlow(entryIndex,cut,cellActivities);

          if((curSF < bestSF) || 
            (aproxEqual(curSF,bestSF) && curAF < bestAF)) {
            const auto techIdsList = getTechIdsList(cut);
            if(techIdsList.size() == 0) continue;
            const SubnetID techCellSubnetID = techIdsList[0];
            bestAF = curAF;
            bestSF = curSF;
            bestCut = cut;
            bestTechCellSubnetID = techCellSubnetID;
          }
        }
        (*bestReplacementMap)[entryIndex].entryIDxs = bestCut.entryIdxs;
        (*bestReplacementMap)[entryIndex].subnetID = bestTechCellSubnetID;
      }
      entryIndex += cell.more;
    }
  }
  PowerMap::PowerMap(){
    computedAF = std::vector<double>();
    computedSF = std::vector<double>();
    computedLevel = std::vector<int64_t>();
    requiredTimes = std::vector<uint32_t>();
    entries = nullptr;
    coneBuilder = nullptr;
  }

  void PowerMap::findBest() {
    Subnet &subnet = Subnet::get(this->subnetID);
    
    entries = new ArrayEntry(subnet.getEntries());
    computedAF.resize(entries->size());
    computedSF.resize(entries->size());
    computedLevel.resize(entries->size());
    requiredTimes.resize(entries->size(),UINT32_MAX);
    coneBuilder = new eda::gate::optimizer2::ConeBuilder(&subnet);

    eda::gate::analyzer::SimulationEstimator simulationEstimator(64);

    eda::gate::analyzer::SwitchActivity switchActivity = 
                                  simulationEstimator.estimate(subnet);

    const std::vector<double> &cellActivities =
                              switchActivity.getActivities();

    // traditionalMapDepthOriented();
    // computeRequiredTimes();
    globalSwitchAreaRecovery(cellActivities);
    clear();
  }

  std::vector<SubnetID> PowerMap::getTechIdsList(const Cut cut) {
    SubnetID coneSubnetID = coneBuilder->getCone(cut).subnetID;
    const auto truthTable = eda::gate::model::evaluate(Subnet::get(coneSubnetID))[0];
    const auto cellList = cellDB->getSubnetIDsByTT(truthTable);
    return cellList;
  }

  void PowerMap::clear() {

      computedAF.clear();
      computedSF.clear();
      computedLevel.clear();
      requiredTimes.clear();
      
      delete entries;
      delete coneBuilder;
      coneBuilder = nullptr;
      entries = nullptr;
  }
} //namespace eda::gate::tech_optimizer