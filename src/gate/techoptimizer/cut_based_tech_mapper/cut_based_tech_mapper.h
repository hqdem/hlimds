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
#include "gate/techoptimizer/cut_based_tech_mapper/strategy/bestReplacement.h"
#include "gate/techoptimizer/cut_based_tech_mapper/strategy/strategy.h"
#include "gate/techoptimizer/library/cellDB.h"

using SubnetID = eda::gate::model::SubnetID;
using EntryIndex = uint64_t;
using CellTypeID = eda::gate::model::CellTypeID;
using CutExtractor = eda::gate::optimizer2::CutExtractor;

using Net = eda::gate::model::Net;

namespace eda::gate::tech_optimizer {

  class CutBasedTechMapper : public BaseMapper {
  public:
    CutBasedTechMapper(CellDB &cellDB);

    void setStrategy(Strategy *strategy) override;

    SubnetID techMap(SubnetID subnetID) override;

    float getArea() const;
    float getDelay() const;

    //TODO: temporarily in the public
    std::map<EntryIndex, BestReplacement> replacementSearch(
      SubnetID subnetID, CutExtractor &cutExtractor);

  private:
    CellDB cellDB;
    Strategy *strategy;

    double area;
    double delay;

    void aigMap(SubnetID subnetID);

    CutExtractor findCuts(SubnetID subnetID);

    SubnetID buildSubnet(SubnetID subnetID, std::map<EntryIndex, BestReplacement> &bestReplacementMap);
  };
} // namespace eda::gate::tech_optimizer
