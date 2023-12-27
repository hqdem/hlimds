//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

// #include "power_map.h"
#include "gate/techoptimizer/power_map/power_map.h"

namespace eda::gate::tech_optimizer{

  double PowerMap::switchFlow(const Subnet& subnet, const std::vector<double> &cellActivities){
      ArrayEntry cells = subnet.getEntries();
      std::vector<double> computedSwitchFlow(cells.size(),0);
      return switchFlowRecursive(cells.size()-1,cellActivities,computedSwitchFlow,cells);
    }

  double PowerMap::switchFlowRecursive(
    const size_t entryIndex,
    const std::vector<double> &cellActivities,
    std::vector<double> &computedSwitchFlow,
    const ArrayEntry &cells){
      if(computedSwitchFlow[entryIndex] != 0.0)return computedSwitchFlow[entryIndex];
      computedSwitchFlow[entryIndex] += cellActivities[entryIndex];
      auto currentCell = cells[entryIndex].cell;

      if(currentCell.isIn()) return computedSwitchFlow[entryIndex];

      for(int i=0;i<currentCell.arity; i++){

        auto link = currentCell.link[i];
        auto leaf = cells[link.idx].cell;
        computedSwitchFlow[entryIndex] +=
         switchFlowRecursive(link.idx,cellActivities,computedSwitchFlow,cells)/double(leaf.refcount); 

      }
      return computedSwitchFlow[entryIndex];
  }

  double PowerMap::areaFlow(Subnet &subnet){
      ArrayEntry cells = subnet.getEntries();
      std::vector<double> computedAreaFlow(cells.size(),0);
      return areaFlowRecursive(cells.size() - 1,computedAreaFlow, cells);
    }

  double PowerMap::areaFlowRecursive(
    const size_t entryIndex,
    std::vector<double> &computedAreaFlow,
    const ArrayEntry &cells){
      if(computedAreaFlow[entryIndex] != 0){ return computedAreaFlow[entryIndex]; }
      auto currentCell = cells[entryIndex].cell;
      if(currentCell.isIn()){
        computedAreaFlow[entryIndex] = 0;
        return 0;
      }
      else {computedAreaFlow[entryIndex] = 1; }

        for(int i=0;i<currentCell.arity; i++){
          auto link = currentCell.link[i];
          auto leaf = cells[link.idx].cell;
          computedAreaFlow[entryIndex] += 
            areaFlowRecursive(link.idx,computedAreaFlow,cells)/double(leaf.refcount); 
      }
      return computedAreaFlow[entryIndex];
    }

  double PowerMap::edgeFlow(
    const size_t entryIndex,
    std::vector<double> &computedEdgeFlow,
    const ArrayEntry &cells){ return 0; }


    void findBest(
      SubnetID subnetId,
      std::map<EntryIndex, BestReplacement> &bestReplacementMap,
      CellDB &cellDB){
        using CutExtractor = eda::gate::optimizer2::CutExtractor;
        const auto& subnet = Subnet::get(subnetId);
        const ArrayEntry entries = subnet.getEntries();
        const unsigned int k = 3;
        std::unordered_map<EntryIndex,Cut> reprCutMap;
        CutExtractor cutExtractor(&subnet, k); 
        ConeBuilder coneBuilder(&subnet);
        for(EntryIndex idx = 0; idx < entries.size(); idx++){

          CutsList cutsList = cutExtractor.getCuts(idx);
          CutsList cuts= {};
          double minAF = 100000.0;
          double delta = 0.001;
          for(size_t i = 0; i < cutsList.size(); i++){
            Cone curCone = coneBuilder.getCone(cutsList[i]);
            Subnet &curSubnet = Subnet::get(curCone.subnetID);
            double curAF = PowerMap::areaFlow(curSubnet);
            if(fabs(curAF - minAF) < delta){cuts.push_back(cutsList[i]);} 
            else if(curAF < minAF){
              cuts.clear();
              cuts.push_back(cutsList[i]);
              minAF = curAF;
            }
          }

          CutsList reprCuts = {};
          double minSF = 100000.0;
          
          eda::gate::analyzer::SimulationEstimator simulationEstimator(64); 
          const auto switchingActivity = simulationEstimator.estimate(subnet);
          const std::vector<double> cellActivities = switchingActivity.getCellActivities();

          for(size_t i = 0; i < cuts.size(); i++){
            Cone curCone = coneBuilder.getCone(cuts[i]);
            Subnet &curSubnet = Subnet::get(curCone.subnetID);
            double curSF = PowerMap::switchFlow(curSubnet,cellActivities);
            if(fabs(curSF - minSF) < delta){reprCuts.push_back(cuts[i]);} 
            else if(curSF < minSF){
              cuts.clear();
              cuts.push_back(cuts[i]);
              minSF = curSF;
            }
          }
          reprCutMap[idx] = reprCuts[0];
        }

        // Mapping
        using EntryIndex = uint64_t;
        std::unordered_set<EntryIndex> M = {};
        std::stack<EntryIndex> F;
        for(EntryIndex idx=0; idx<entries.size();idx++){
          if(entries[idx].cell.isPO()){
            M.insert(idx);
            F.push(idx);
          }
        }
        while(!F.empty()){
          EntryIndex curIdx = F.top();
          F.pop();
          Cut cut = reprCutMap[curIdx];
          for(const auto& nodeIdx : cut.entryIdxs){
            if(M.count(nodeIdx) == 1 || entries[nodeIdx].cell.isPI()){ continue; }
            M.insert(nodeIdx);
            F.push(nodeIdx);
          }

        }
        // M is the set of nodes used for final mapping
      }

  // void switchFlowTest1(){
  //   std::cout << "my test\n";

  //   auto subnetId = eda::gate::model::randomSubnet(3,1,7,2,3);
  //   const auto &subnet = Subnet::get(subnetId);
  //   std::cout << subnet << std::endl;
  //   const auto cells = subnet.getEntries();
  //   std::vector<double> cellActivities({0.1, 0.2, 0.3, 0.1, 0.2, 0.3, 0.25});
  //   std::vector<double> computedSwitchFlow({0,0,0,0,0,0,0});
  //   double r=0;
  //   for(int i=0; i < 7;i++){
  //     r = PowerMap::switchFlow(i,cellActivities,computedSwitchFlow,subnet,cells);
  //   }
  //   std::cout <<r << std::endl;
  // }

} //namespace eda::gate::tech_optimizer


