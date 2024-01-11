//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

// #include "power_map.h"
#include "gate/techoptimizer/power_map/power_map.h"
#include <algorithm>
namespace eda::gate::tech_optimizer{

  double PowerMap::switchFlow(
    const ArrayEntry &cells,
    const EntryIndex entryIndex,
    const Cut &cut
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
    const Cut &cut
  ){
    double area = 1;
    auto currentCell  = cells[entryIndex].cell;
    if (currentCell.isIn()){
      computedAreaFlow[entryIndex] = 0;
      return 0;
    }
    double af = area;
    for (const auto &leafIdx : cut.entryIdxs){
      const auto leaf = cells[leafIdx].cell;
      if(leaf.isIn())continue;
      af += computedAreaFlow[leafIdx]/(leaf.refcount);
    }
    computedAreaFlow[entryIndex] = af;
    return af;
  }

  void PowerMap::findBest(
    EntryIndex entryIndex,
    const CutsList &cutsList,
    std::map<EntryIndex,BestReplacement> &bestReplacementMap,
    CellDB &cellDB,
    SubnetID subnetId){
      if(this->initialized == false)this->init(subnetId);
      std::vector<BestReplacementPower> sortedCuts(cutsList.size());
      ArrayEntry entries = Subnet::get(subnetId).getEntries();
      for(size_t cutIdx=0; cutIdx < cutsList.size(); cutIdx++){
        BestReplacementPower repl;
        repl.cutIdx = cutIdx;
        repl.areaFlow = areaFlow(entries,entryIndex,cutsList[cutIdx]);
        repl.switchFlow = switchFlow(entries,entryIndex,cutsList[cutIdx]);
        sortedCuts[cutIdx] = repl;
      }
      std::sort(sortedCuts.begin(),sortedCuts.end(),costAreaSwitch);
      std::reverse(sortedCuts.begin(),sortedCuts.end());

      ConeBuilder coneBuilder(&Subnet::get(subnetId));

      for(size_t replIdx=0; replIdx < sortedCuts.size(); replIdx++){
        const Cut &curCut = cutsList[sortedCuts[replIdx].cutIdx];
        SubnetID coneSubnetID = coneBuilder.getCone(curCut).subnetID;
        auto truthTable = eda::gate::model::evaluate(Subnet::get(coneSubnetID));
        const auto& cellList = cellDB.getSubnetIDsByTT(truthTable);
        if(cellList.size() == 0) continue;
        const SubnetID currentSubnetID = cellList[0];
        sortedCuts[replIdx].subnetID = currentSubnetID;
        bestReplacementMap[entryIndex] = sortedCuts[replIdx];
        
        break;
      }
    }


  double PowerMap::edgeFlow(
    const size_t entryIndex,
    std::vector<double> &computedEdgeFlow,
    const ArrayEntry &cells){ return 0; }


    
} //namespace eda::gate::tech_optimizer


