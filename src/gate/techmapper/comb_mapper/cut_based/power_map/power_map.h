//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/analyzer/switching_activity.h"
#include "gate/optimizer/cone_builder.h"
#include "gate/techmapper/comb_mapper/cut_based/cut_based_mapper.h"

namespace eda::gate::techmapper {
struct PowerMetrics{
  double af = MAXFLOAT;
  double sf = MAXFLOAT;
  uint32_t cutIdx = 0;
  uint32_t level = 0;
  uint32_t requiredTime = UINT32_MAX;
  uint32_t refCounter = 0;
};

class PowerMap : public CutBaseMapper {
  using Subnet = eda::gate::model::Subnet;
  using Entry = Subnet::Entry;
  using ArrayEntry = eda::gate::model::Array<Entry>;
  using Cut = eda::gate::optimizer::CutExtractor::Cut;
  using CutsList = std::vector<Cut>;
  using ConeBuilder = eda::gate::optimizer::ConeBuilder;
  using Cone = ConeBuilder::Cone;

  public:
    PowerMap();
  protected:
    void findBest() override;
    ~PowerMap(){
      clear();
    }

  private:
    double getArea(const Cut &cut);
    double areaFlow(const EntryIndex entryIndex,const Cut &cut);

    double getSwitching(const Cut &cut);
    double switchFlow(const EntryIndex entryIndex, const Cut &cut);

    double getCellPower(const Cut &cut, const SubnetID &techCellSubnetId);
    double getCellArea(const Cut &cut, const SubnetID &techCellSubnetId);
 
    uint32_t getLevel(const EntryIndex entryIdx);
    uint32_t getLevel(const Cut &cut);
    uint32_t getLevel(const std::vector<uint64_t> &entryIdxs);

    void findCutMinimizingDepth(const EntryIndex entryIndex);

    void depthOrientedMap();
    void globalSwitchAreaRecovery();
    void localSwitchAreaRecovery();

    double exactAreaRef(const Cut &cut);
    double exactAreaDeref(const Cut &cut);

    double exactSwitchRef(const Cut &cut);
    double exactSwitchDeref(const Cut &cut);

    double exactArea(EntryIndex entryIndex, const Cut &cut);
    double exactSwitch(EntryIndex entryIndex, const Cut &cut);

    bool cutIsRepr(EntryIndex entryIndex,const Cut &cut);
    uint32_t findLatestPoArivalTime();
    void computeRequiredTimes();

    std::vector<SubnetID> getTechIdsList(const Cut cut);
    SubnetID getBestTechCellSubnetId(const Cut &cut);
    SubnetID getBestAreaTechCellSubnetId(const Cut &cut);
    void saveBestReplacement(EntryIndex entryIndex, const Cut &cut,
                             const SubnetID techSubnetId);

    void ref(const EntryIndex &entryIndex);
    void deref(const EntryIndex &entryIndex);

    void init();
    void clear();
    
    std::vector<PowerMetrics> metrics;

    eda::gate::analyzer::SwitchActivity switchActivity 
      = eda::gate::analyzer::SwitchActivity({});
    eda::gate::optimizer::ConeBuilder *coneBuilder;
    ArrayEntry *entries;
};
} // namespace eda::gate::techmapper
