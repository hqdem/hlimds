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

namespace eda::gate::tech_optimizer {

  using SubnetID = eda::gate::model::SubnetID;
  using CellTypeID = eda::gate::model::CellTypeID;

  class CutBasedTechMapper2 {
  public:
    CutBasedTechMapper2();

    void set(SubnetID subnetID, CellDB cellDB, Strategy2 *strategy);

    SubnetID techMap();

    float getArea() const;
    float getDelay() const;

  private:
    SubnetID subnetID;
    CellDB cellDB;
    Strategy2 *strategy;

    double area;
    double delay;

    void aigMap();
    void findCuts(CutStorage &cutStorage);
    void replacementSearch();
    void buildNet( 
        std::unordered_map<GateID, Replacement> &bestSubstitutions);
    void printNet(const Net &model2);
  };
} // namespace eda::gate::tech_optimizer
