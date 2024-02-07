//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

// #include "power_map.h"
#include "gate/techoptimizer/mapper/cut_base/power_map/power_map.h"
#include <algorithm>
namespace eda::gate::tech_optimizer{

  double PowerMap::switchFlow(
    const ArrayEntry &cells,
    const EntryIndex entryIndex,
    const Cut &cut,
    std::vector<double> &computedSwitchFlow,
    const std::vector<double> &cellActivities
  ){
    double sf = cellActivities[entryIndex];
    auto currentCell  = cells[entryIndex].cell;
    if (!currentCell.isIn()){
      for (const auto &leafIdx : cut.entryIdxs){
        const auto leaf = cells[leafIdx].cell;
        if(leaf.isIn()){
          computedSwitchFlow[leafIdx] = cellActivities[leafIdx];
        } 
        sf += computedSwitchFlow[leafIdx]/(cells[leafIdx].cell.refcount);
      }
    }
    computedSwitchFlow[entryIndex] = sf;
    return sf;
  }
  
  double PowerMap::areaFlow(
    const ArrayEntry &cells,
    const EntryIndex entryIndex,
    const Cut &cut,
    std::vector<double> &computedAreaFlow
  ){
    double af = 1;
    auto currentCell  = cells[entryIndex].cell;
    if (currentCell.isIn()){
      computedAreaFlow[entryIndex] = 0;
      return 0;
    }   
    for (const auto &leafIdx : cut.entryIdxs){ 
      const auto leaf = cells[leafIdx].cell;
      if(leaf.isIn())continue;
      af += computedAreaFlow[leafIdx]/(leaf.refcount);
    }
    computedAreaFlow[entryIndex] = af;
    return af;
  }

  int64_t getLevel(const Cut &cut, const Subnet &subnet){
    int64_t levelMax = -99999999;
    for(const EntryIndex &leaf: cut.entryIdxs){
      
      // levelMax= max(levelMax,subnet.getPathLength().second);
    }
    return levelMax;
  }

  Cut PowerMap::findCutMinimizingDepth(const EntryIndex entryIndex, const Subnet &subnet){
    Cut *cutBest = nullptr;
    
    CutsList cutsList = cutExtractor->getCuts(entryIndex);
    for(const Cut &cut : cutsList){
      if (cutBest == nullptr || getLevel(*cutBest,subnet) > getLevel(cut,subnet)){
        *cutBest = cut;
      }
    }
    return *cutBest;
  }


  bool aproxEqual(const double &a , const double &b){
    return fabs(a-b) < 0.0001;
  }
  
  void PowerMap::findBest(){
    Subnet &subnet = Subnet::get(this->subnetID);
    ArrayEntry entries = subnet.getEntries();

    std::vector<double> computedAF(entries.size()),
                       computedSF(entries.size());

    //
    eda::gate::analyzer::SimulationEstimator simulationEstimator(64);
    eda::gate::analyzer::SwitchActivity switchActivity =
                                  simulationEstimator.estimate(subnet);
    const std::vector<double> &cellActivities = 
                                  switchActivity.getCellActivities();

    eda::gate::optimizer2::ConeBuilder coneBuilder(&subnet);
    
    for (uint64_t entryIndex = 0; entryIndex < std::size(entries);entryIndex++) {
    auto cell = entries[entryIndex].cell;

      if(!cell.isAnd()){
        addNotAnAndToTheMap(entryIndex,cell);
      } else {  
        CutsList cutsList = cutExtractor->getCuts(entryIndex);
        double bestAF = MAXFLOAT;
        double bestSF = MAXFLOAT;
        Cut bestCut = Cut();
        SubnetID bestTechCellSubnetID = 0;

        for(const Cut &cut : cutsList){
          if(cut.entryIdxs.size() == 1)continue;
          double curAF = areaFlow(entries,entryIndex,cut,computedAF);
          double curSF = switchFlow(entries,entryIndex,cut,computedSF,cellActivities);

          if((curAF < bestAF) || 
            (aproxEqual(curAF,bestAF) && curSF < bestSF)){

            SubnetID coneSubnetID = coneBuilder.getCone(cut).subnetID;
            auto truthTable = eda::gate::model::evaluate(Subnet::get(coneSubnetID));
            const auto& cellList = cellDB->getSubnetIDsByTT(truthTable);
            if(cellList.size() == 0) continue;
            const SubnetID techCellSubnetID = cellList[0];
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

  void PowerMap::addInputToTheMap(EntryIndex entryIndex) {
  BestReplacement bestReplacement{true};
  (*bestReplacementMap)[entryIndex] = bestReplacement;
  }
  void PowerMap::addZeroToTheMap(EntryIndex entryIndex) {
    BestReplacement bestReplacement{};
    bestReplacement.isZero = true;
    (*bestReplacementMap)[entryIndex] = bestReplacement;
  }
  void PowerMap::addOneToTheMap(EntryIndex entryIndex) {
    BestReplacement bestReplacement{};
    bestReplacement.isOne = true;
    (*bestReplacementMap)[entryIndex] = bestReplacement;
  }
  void PowerMap::addOutToTheMap(EntryIndex entryIndex,Subnet::Cell &cell) {
    BestReplacement bestReplacement{false, true};
    bestReplacement.entryIDxs.insert(cell.link[0].idx);
    (*bestReplacementMap)[entryIndex] = bestReplacement;
  }

  void PowerMap::addNotAnAndToTheMap(EntryIndex entryIndex, model::Subnet::Cell &cell){
    if (cell.isIn()) {
      addInputToTheMap(entryIndex);
    } else if (cell.isOne()) {
      addOneToTheMap(entryIndex);
    } else if (cell.isZero()) {
      addZeroToTheMap(entryIndex);
    } else if (cell.isOut()) {
      addOutToTheMap(entryIndex, cell);
    }
  }
    
} //namespace eda::gate::tech_optimizer


