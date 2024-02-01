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
#include "gate/techoptimizer/mapper/cut_base/cut_base_mapper.h"

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
  using CutsList = std::vector<Cut>;
  using ConeBuilder = eda::gate::optimizer2::ConeBuilder;
  using Cone = ConeBuilder::Cone;
  using EntryMap = std::unordered_map<uint64_t, uint64_t>;

  class BestReplacementPower : public BestReplacement{
    public:
      BestReplacementPower(){
        areaFlow = 1000000.0;
        switchFlow = 1000000.0;
      };
      double switchFlow;
      double areaFlow;
  };

  inline bool costAreaSwitch(const BestReplacementPower& left, const BestReplacementPower& right){
    if(left.areaFlow == right.areaFlow)return left.switchFlow < right.switchFlow;
    return left.areaFlow < right.areaFlow;
  }


class PowerMap : public CutBaseMapper{
  protected:
    
    void findBest() override;
  private:

    double areaFlow(const ArrayEntry &cells,
                    const EntryIndex entryIndex,const Cut &cut,
                    std::vector<double> &computedAreaFlow);

    double switchFlow(const ArrayEntry &cells,
                      const EntryIndex entryIndex, const Cut &cut,
                      std::vector<double> &computedSwitchFlow,
                      const std::vector<double> &cellActivities);

    void addNotAnAndToTheMap(EntryIndex entryIndex,
                      model::Subnet::Cell &cell);

    void addInputToTheMap(EntryIndex entryIndex);
    void addZeroToTheMap(EntryIndex entryIndex);
    void addOneToTheMap(EntryIndex entryIndex);
    void addOutToTheMap(EntryIndex entryIndex,
                      model::Subnet::Cell &cell);
};

} // namespace eda::gate::tech_optimizer
