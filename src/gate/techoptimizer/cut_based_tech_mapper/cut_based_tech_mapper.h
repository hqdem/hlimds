//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/celltype.h"
#include "gate/model2/net.h"
#include "gate/optimizer2/cut_extractor.h"
#include "gate/techoptimizer/baseMapper.h"
#include "gate/techoptimizer/library/cellDB.h"

using SubnetID = eda::gate::model::SubnetID;
using EntryIndex = uint64_t;
using CellTypeID = eda::gate::model::CellTypeID;
using CutExtractor = eda::gate::optimizer2::CutExtractor;

using Net = eda::gate::model::Net;

namespace eda::gate::tech_optimizer {

  class CutBasedTechMapper : public BaseMapper {
  public:
    CutBasedTechMapper(CellDB *cellDB);

    void setStrategy(Strategy *strategy,
        std::map<uint64_t, BestReplacement> *bestReplacementMap) override;

    SubnetID techMap(SubnetID subnetID) override;

    float getArea() const;
    float getDelay() const;

    ~CutBasedTechMapper() override {
      delete strategy;
      delete bestReplacementMap;
    }

  private:
    CellDB *cellDB;
    Strategy *strategy;
    std::map<EntryIndex, BestReplacement> *bestReplacementMap;

    double area;
    double delay;

    SubnetID aigMap(SubnetID subnetID);

    void replacementSearch(SubnetID subnetID);

    SubnetID buildSubnet(SubnetID subnetID);

    void singlePassSearch(SubnetID subnetId,
                          CutExtractor &cutExtractor);

    void addInputToTheMap(EntryIndex entryIndex);
    void addZeroToTheMap(EntryIndex entryIndex);
    void addOneToTheMap(EntryIndex entryIndex);
    void addOutToTheMap(EntryIndex entryIndex,
                        model::Subnet::Cell &cell);
  };
} // namespace eda::gate::tech_optimizer
