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

  double PowerMap::switchFlow(
    const size_t entryIndex,
    const std::vector<double> &cellActivities,
    std::vector<double> &computedSwitchFlow,
    const Subnet &subnet,
    const ArrayEntry &cells){
      computedSwitchFlow[entryIndex] += cellActivities[entryIndex];
      auto currentCell = cells[entryIndex].cell;
      if(currentCell.isIn()) return computedSwitchFlow[entryIndex];

      for(int i=0;i<currentCell.arity; i++){
        auto link = subnet.getLink(entryIndex,i);
        auto leaf = cells[link.idx].cell;
        computedSwitchFlow[entryIndex] += computedSwitchFlow[link.idx] / double(leaf.refcount); 

      }
      return computedSwitchFlow[entryIndex];
  }

  double PowerMap::areaFlow(
        const size_t entryIndex,
        std::vector<double> &computedAreaFlow,
        const Subnet &subnet,
        const ArrayEntry &cells){ return 0; }

  double PowerMap::edgeFlow(
        const size_t entryIndex,
        std::vector<double> &computedEdgeFlow,
        const Subnet &subnet,
        const ArrayEntry &cells){ return 0; }

  void switchFlowTest1(){
    std::cout << "my test\n";

    auto subnetId = eda::gate::model::randomSubnet(3,1,7,2,3);
    const auto &subnet = Subnet::get(subnetId);
    std::cout << subnet << std::endl;
    const auto cells = subnet.getEntries();
    std::vector<double> cellActivities({0.1, 0.2, 0.3, 0.1, 0.2, 0.3, 0.25});
    std::vector<double> computedSwitchFlow({0,0,0,0,0,0,0});
    double r=0;
    for(int i=0; i < 7;i++){
      r = PowerMap::switchFlow(i,cellActivities,computedSwitchFlow,subnet,cells);
    }
    std::cout <<r << std::endl;
  }

} //namespace eda::gate::tech_optimizer


