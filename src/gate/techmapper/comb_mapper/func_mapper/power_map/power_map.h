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
#include "gate/techmapper/comb_mapper/func_mapper/func_mapper.h"

namespace eda::gate::techmapper {
struct PowerMetrics{
  double af = MAXFLOAT;
  double sf = MAXFLOAT;
  uint32_t cutIdx = 0;
  uint32_t level = 0;
  uint32_t requiredTime = UINT32_MAX;
  uint32_t refCounter = 0;
};

class PowerMap : public FuncMapper {
  using Subnet = model::Subnet;
  using Entry = Subnet::Entry;
  using ArrayEntry = model::Array<Entry>;
  using Cut = optimizer::CutExtractor::Cut;
  using CutsList = std::vector<Cut>;
  using ConeBuilder = optimizer::ConeBuilder;
  using Cone = ConeBuilder::Cone;

  public:
    PowerMap();
    void map(const SubnetID subnetID,
           const CellDB &cellDB,
           const SDC &sdc,
           Mapping &mapping) override;
    virtual ~PowerMap() {
      clear();
    }

  private:
    double getArea(const Cut &cut) const;
    double getAreaFlow(const EntryIndex entryIndex,const Cut &cut) const;

    double getSwitching(const Cut &cut) const;
    double calcSwitchFlow(const EntryIndex entryIndex, const Cut &cut);

    double getCellPower(const Cut &cut, const SubnetID &techCellSubnetId, const CellDB &cellDB) const;
    double getCellArea(const SubnetID &techCellSubnetId, const CellDB &cellDB) const;

    uint32_t getLevel(const EntryIndex entryIdx) const;
    uint32_t getLevel(const Cut &cut) const;
    uint32_t getLevel(const std::vector<uint64_t> &entryIdxs) const;

    void findCutMinimizingDepth(const EntryIndex entryIndex,
           const CellDB &cellDB, Mapping &mapping);

    void depthOrientedMap(const CellDB &cellDB, Mapping &mapping);
    void globalSwitchAreaRecovery(const CellDB &cellDB, Mapping &mapping);
    void localSwitchAreaRecovery(const CellDB &cellDB, Mapping &mapping);

    double exactAreaRef(const Cut &cut, const Mapping &mapping);
    double exactAreaDeref(const Cut &cut, const Mapping &mapping);

    //double exactSwitchRef(const Cut &cut, Mapping &mapping);
    //double exactSwitchDeref(const Cut &cut, Mapping &mapping);

    double exactArea(EntryIndex entryIndex, const Cut &cut,
             const Mapping &mapping);
    double exactSwitch(EntryIndex entryIndex, const Cut &cut,
             const Mapping &mapping) const;

    bool cutIsRepr(EntryIndex entryIndex, const Cut &cut,
             const Mapping &mapping) const;
    uint32_t findLatestPoArivalTime(const Mapping &mapping) const;
    void computeRequiredTimes(const Mapping &mapping);

    std::vector<SubnetID> getTechIdsList(const Cut &cut, const CellDB &cellDB);
    SubnetID getBestTechCellSubnetId(const Cut &cut, const CellDB &cellDB);
    SubnetID getBestAreaTechCellSubnetId(const Cut &cut, const CellDB &cellDB);
    void saveMappingItem(const EntryIndex entryIndex, const Cut &cut,
                         const SubnetID techSubnetId, Mapping &mapping);

    void ref(const EntryIndex &entryIndex, const Mapping &mapping);
    void deref(const EntryIndex &entryIndex, const Mapping &mapping);

    void init(Mapping &mapping);
    void clear();

    std::vector<PowerMetrics> metrics;

    eda::gate::analyzer::SwitchActivity switchActivity
      = eda::gate::analyzer::SwitchActivity({}, {});
    eda::gate::optimizer::ConeBuilder *coneBuilder;
    ArrayEntry *entries;
  optimizer::CutExtractor *cutExtractor;
};
} // namespace eda::gate::techmapper
