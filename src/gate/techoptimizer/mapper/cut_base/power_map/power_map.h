//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#pragma once

#include "gate/optimizer2/cone_builder.h"
#include "gate/techoptimizer/mapper/cut_base/cut_base_mapper.h"

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

class PowerMap : public CutBaseMapper{
  public:
    PowerMap() = default;
  protected:
    void findBest() override;
    
    ~PowerMap(){
      clear();
    }
    
  private:

    double areaFlow(const EntryIndex entryIndex,const Cut &cut);

    double switchFlow(const EntryIndex entryIndex, const Cut &cut,
                      const std::vector<double> &cellActivities);

    int64_t getLevel(const EntryIndex entryIdx);
    int64_t getLevel(const Cut &cut);
    BestReplacement findCutMinimizingDepth(const EntryIndex entryIndex);
    
    void traditionalMapDepthOriented();
    void globalSwitchAreaRecovery(const std::vector<double> &cellActivities);
    void computeRequiredTimes();
    std::vector<SubnetID> getTechIdsList(const Cut cut);

    void clear();

    std::vector<double> *computedAF = nullptr;
    std::vector<double> *computedSF = nullptr;
    std::vector<int64_t> *computedLevel = nullptr;
    std::vector<uint32_t> *requiredTimes = nullptr;
    eda::gate::optimizer2::ConeBuilder *coneBuilder = nullptr;
    ArrayEntry *entries = nullptr;
};

} // namespace eda::gate::tech_optimizer
