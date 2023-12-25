//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#pragma once
#include "gate/model2/array.h"
#include "gate/model2/celltype.h"
#include "gate/model2/object.h"
#include "gate/model2/subnet.h"
#include "gate/model2/utils/subnet_random.h"
#include "gate/optimizer2/cut_extractor.h"
#include "gate/techoptimizer/cut_based_tech_mapper/strategy/strategy.h"


#include <iostream>
#include <map>
#include <vector>

namespace eda::gate::tech_optimizer {

  using Subnet = eda::gate::model::Subnet;
  using Entry = Subnet::Entry;
  using ArrayEntry = eda::gate::model::Array<Entry>;
  using SubnetBuilder = eda::gate::model::SubnetBuilder;
  using Cut = eda::gate::optimizer2::CutExtractor::Cut;

  class PowerMap : public Strategy{
    public:

      void findBest(
        EntryIndex entryIndex,
        const CutsList &CutsList, 
        std::map<EntryIndex, BestReplacement> &bestReplacementMap, 
        CellDB &cellDB, 
        ArrayEntry &entries) = 0;
    // private:

      static double switchFlow(
        const size_t entryIndex,
        const Cut &cut,
        const std::vector<double> &cellActivities,
        // std::vector<double> &computedSwitchFlow,
        // const Subnet &subnet,
        const ArrayEntry &cells);

      double areaFlow(
        const size_t entryIndex,
        std::vector<double> &computedAreaFlow,
        const Subnet &subnet,
        const ArrayEntry &cells);
        
      double edgeFlow(
        const size_t entryIndex,
        std::vector<double> &computedEdgeFlow,
        const Subnet &subnet,
        const ArrayEntry &cells);

      static double switchFlowRecursive(
        const size_t entryIndex,
        const Cut &cut,
        const std::vector<double> &cellActivities,
        std::vector<double> &computedSwitchFlow,
        // const Subnet &subnet,
        const ArrayEntry &cells);
      
  };

  void switchFlowTest1();
} // namespace eda::gate::tech_optimizer
