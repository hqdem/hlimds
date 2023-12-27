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

  class PowerMap : public Strategy{
    public:

      void findBest(
        EntryIndex entryIndex,
        const CutsList &CutsList, 
        std::map<EntryIndex, BestReplacement> &bestReplacementMap, 
        CellDB &cellDB, 
        ArrayEntry &entries) = 0;
      
      void findBest(
        SubnetID subnetId,
        std::map<EntryIndex, BestReplacement> &bestReplacementMap,
        CellDB &cellDB);
    // private:

      static double switchFlow(const Subnet &subnet, const std::vector<double> &cellActivities );

        
      static double switchFlowRecursive(
        const size_t entryIndex,
        const std::vector<double> &cellActivities,
        std::vector<double> &computedSwitchFlow,
        const ArrayEntry &cells);

      static double areaFlow( Subnet &subnet);

      static double areaFlowRecursive(
        const size_t entryIndex,
        std::vector<double> &computedAreaFlow,
        const ArrayEntry &cells);
        
      double edgeFlow(
        const size_t entryIndex,
        std::vector<double> &computedEdgeFlow,
        const ArrayEntry &cells);


      
  };

  void switchFlowTest1();
} // namespace eda::gate::tech_optimizer
