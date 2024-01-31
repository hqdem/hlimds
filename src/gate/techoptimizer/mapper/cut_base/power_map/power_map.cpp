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

    bool aproxEqual(const double &a , const double &b){
      return fabs(a-b) < 0.0001;
    }
    void PowerMap::findBest(
      EntryIndex entryIndex,
      const CutsList &cutsList,
      std::map<EntryIndex,BestReplacement> &bestReplacementMap,
      CellDB &cellDB,
      SubnetID subnetId){
      
      if(this->initialized == false)this->init(subnetId);
      BestReplacementPower bestRepl = BestReplacementPower();
      ArrayEntry entries = Subnet::get(subnetId).getEntries();
      ConeBuilder coneBuilder(&Subnet::get(subnetId));
      for(const Cut &cut : cutsList){
        if(cut.entryIdxs.size() == 1)continue;
        double curAF = areaFlow(entries,entryIndex,cut);
        // double curSF = switchFlow(entries,entryIndex,cut);
        double curSF = 0;

        if((curAF < bestRepl.areaFlow) || 
          (aproxEqual(curAF,bestRepl.areaFlow) && curSF < bestRepl.switchFlow)){

          SubnetID coneSubnetID = coneBuilder.getCone(cut).subnetID;
          auto truthTable = eda::gate::model::evaluate(Subnet::get(coneSubnetID));
          const auto& cellList = cellDB.getSubnetIDsByTT(truthTable);
          if(cellList.size() == 0) continue;
          const SubnetID currentSubnetID = cellList[0];

          bestRepl.areaFlow = curAF;
          bestRepl.switchFlow = curSF;
          bestRepl.entryIDxs = cut.entryIdxs;
          bestRepl.subnetID = currentSubnetID;

        }

      }
      // std::cout << entryIndex << ": AF " << bestRepl.areaFlow << ": ";
      // for (const auto& entry :  bestRepl.entryIDxs){
      //   std::cout << entry << " " ; 
      // }
      // std:: cout <<std::endl;
      
      bestReplacementMap[entryIndex] = bestRepl;
  }

  void PowerMap::findBest(
    SubnetID subnetID,
    CutExtractor &cutExtractor,
    CellDB &cellDB,
    std::map<EntryIndex, BestReplacement>
    &bestReplacementMap){};


  double PowerMap::edgeFlow(
    const size_t entryIndex,
    std::vector<double> &computedEdgeFlow,
    const ArrayEntry &cells){ return 0; }


    
} //namespace eda::gate::tech_optimizer


