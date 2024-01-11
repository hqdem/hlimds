//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#pragma once

#include "gate/analyzer/simulation_estimator.h"
#include "gate/model2/array.h"
#include "gate/model2/celltype.h"
#include "gate/model2/object.h"
#include "gate/model2/subnet.h"
#include "gate/model2/utils/subnet_random.h"
#include "gate/model2/utils/subnet_truth_table.h"
#include "gate/optimizer2/cone_builder.h"
#include "gate/optimizer2/cut_extractor.h"
#include "gate/techoptimizer/cut_based_tech_mapper/strategy/strategy.h"

#include <iostream>
#include <list>
#include <map>
#include <stack>
#include <unordered_map>

namespace eda::gate::tech_optimizer {

  using Subnet = eda::gate::model::Subnet;
  using Entry = Subnet::Entry;
  using ArrayEntry = eda::gate::model::Array<Entry>;
  using SubnetBuilder = eda::gate::model::SubnetBuilder;
  using Cut = eda::gate::optimizer2::CutExtractor::Cut;
  using ConeBuilder = eda::gate::optimizer2::ConeBuilder;
  using Cone = ConeBuilder::Cone;
  using EntryMap = std::unordered_map<uint64_t, uint64_t>;

  class BestReplacementPower : public BestReplacement{
    public:
      BestReplacementPower() = default;
      double switchFlow;
      double areaFlow;
      size_t cutIdx;
  };

  inline bool costAreaSwitch(const BestReplacementPower& left, const BestReplacementPower& right){
    if(left.areaFlow == right.areaFlow)return left.switchFlow < right.switchFlow;
    return left.areaFlow < right.areaFlow;
  }

  // bool operator<(const BestReplacementPower& left, const BestReplacementPower& right){
  //   if(left.areaFlow == right.areaFlow)return left.switchFlow < right.switchFlow;
  //   return left.areaFlow < right.areaFlow;
  // } 
  class PowerMap : public Strategy{
    public:

      PowerMap(){
        this->initialized = false;
      }

      void findBest(
        EntryIndex entryIndex,
        const CutsList &CutsList,
        std::map<EntryIndex,BestReplacement> &bestReplacementMap,
        CellDB &cellDB,
        SubnetID subnetId);

      double switchFlow(
        const ArrayEntry &cells,
        const EntryIndex entryIndex,
        const Cut &cut
      );

      double areaFlow(
        const ArrayEntry &cells,
        const EntryIndex entryIndex,
        const Cut &cut
      );
        
      double edgeFlow(
        const size_t entryIndex,
        std::vector<double> &computedEdgeFlow,
        const ArrayEntry &cells);

    private:

      // init is called during first call of findBest()
      void init(SubnetID subnetId){
        this->initialized = true;
        this->subnetId = subnetId;
        auto& subnet = Subnet::get(subnetId);
        const auto& entries = subnet.getEntries();
        size_t subnetSize = entries.size();

        this->computedAreaFlow = std::vector<double> (subnetSize, 0);
        this->computedSwitchFlow = std::vector<double> (subnetSize, 0);

        eda::gate::analyzer::SimulationEstimator simulationEstimator(64);
        this->cellActivities = simulationEstimator.estimate(subnet).getCellActivities();
      }

      void reset(){
        this->initialized =false;
      }

      SubnetID subnetId;
      std::vector<double> cellActivities;
      std::vector<double> computedAreaFlow;
      std::vector<double> computedSwitchFlow;
      bool initialized;
  };

} // namespace eda::gate::tech_optimizer
