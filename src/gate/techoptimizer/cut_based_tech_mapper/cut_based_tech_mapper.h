//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gnet.h"
#include "gate/model2/celltype.h"
#include "gate/model2/net.h"
#include "gate/optimizer/cut_storage.h"
#include "gate/optimizer/net_substitute.h"
#include "gate/optimizer/rwdatabase.h"
#include "gate/techoptimizer/cut_based_tech_mapper/replacement_struct.h"
#include "gate/techoptimizer/cut_based_tech_mapper/strategy/strategy.h"

namespace eda::gate::tech_optimizer {

  using GNet = eda::gate::model::GNet;
  using Net = eda::gate::model::Net;
  using GateID = GNet::GateId;
  using CutStorage = eda::gate::optimizer::CutStorage;
  using CellTypeID = eda::gate::model::CellTypeID;

  class CutBasedTechMapper {
  public:
    CutBasedTechMapper(const std::string &libertyPath);
    CutBasedTechMapper(eda::gate::optimizer::SQLiteRWDatabase &rwdb, 
        std::unordered_map<std::string, CellTypeID> &cellTypeMap);

    GNet *techMap(GNet *net, Strategy *strategy, bool aig);
    float getArea() const;
    float getDelay() const;

  private:
    std::string dbPath = "rwtest.db";
    eda::gate::optimizer::SQLiteRWDatabase rwdb;
    std::unordered_map<std::string, CellTypeID> cellTypeMap;

    double area;
    double delay;

    void aigMap(GNet *&net);
    void findCuts(GNet *net, CutStorage &cutStorage);
    void replacementSearch(GNet *net, Strategy *strategy, 
        std::unordered_map<GateID, Replacement> &bestSubstitutions,
        CutStorage &cutStorage, 
        std::unordered_map<std::string, CellTypeID> &cellTypeMap);
    const Net &createModel2(GNet *net, 
        std::unordered_map<GateID, Replacement> &bestSubstitutions);
    void printNet(const Net &model2);
  };
} // namespace eda::gate::tech_optimizer
