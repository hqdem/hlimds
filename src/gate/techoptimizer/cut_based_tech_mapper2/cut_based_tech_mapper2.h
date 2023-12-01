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
#include "gate/techoptimizer/library/cellDB.h"
#include "gate/techoptimizer/cut_based_tech_mapper2/strategy/strategy.h"
#include "gate/optimizer2/cut_extractor.h"

namespace eda::gate::tech_optimizer {

  using SubnetID = model::SubnetID;
  using CellID = uint64_t;
  using CellTypeID = model::CellTypeID;
  using CutExtractor = optimizer2::CutExtractor;

  using Net = model::Net;

  class CutBasedTechMapper2 {
  public:
    CutBasedTechMapper2();

    void set(CellDB cellDB, Strategy2 *strategy);

    SubnetID techMap(SubnetID subnetID);

    float getArea() const;
    float getDelay() const;

  private:
    SubnetID subnetID;

    CellDB cellDB;
    Strategy2 *strategy;

    double area;
    double delay;

    // Dosnt work yet for model2
    void aigMap(SubnetID subnetID);

    CutExtractor findCuts(SubnetID subnetID);

    std::map<CellID, BestReplacement> replacementSearch(
      SubnetID subnetID, CutExtractor cutExtractor);

    SubnetID buildSubnet(std::map<CellID, BestReplacement> &bestReplacementMap);

    const Net &buildModel2(std::map<CellID, BestReplacement> &bestReplacementMap);
    void printNet(const Net &model2);
  };
} // namespace eda::gate::tech_optimizer
