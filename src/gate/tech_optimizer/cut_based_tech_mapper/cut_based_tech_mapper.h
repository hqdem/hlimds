//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gnet.h"
#include "gate/optimizer/cut_storage.h"
#include "gate/optimizer/rwdatabase.h"
#include "gate/tech_optimizer/cut_based_tech_mapper/replacement_struct.h"
#include "gate/tech_optimizer/cut_based_tech_mapper/strategy/strategy.h"

namespace eda::gate::tech_optimizer {

  using GNet = eda::gate::model::GNet;
  using GateID = GNet::GateId;
  using CutStorage = eda::gate::optimizer::CutStorage;

  class CutBasedTechMapper {
  public:
    CutBasedTechMapper(const std::string &libertyPath);
    CutBasedTechMapper(eda::gate::optimizer::SQLiteRWDatabase &rwdb);

    GNet *techMap(GNet *net, Strategy *strategy, bool aig);
    float getArea() const;
    float getDelay() const;

  private:
    CutStorage cutStorage;
    std::unordered_map<GateID, double> gatesDelay;
    std::unordered_map<GateID, Replacement> bestReplacement;

    std::string dbPath = "rwtest.db";
    eda::gate::optimizer::SQLiteRWDatabase rwdb;

    double area;
    double delay;

    void aigMap(GNet *&net);
    void findCuts(GNet *net);
    void replacementSearch(GNet *net, Strategy *strategy);
    void replacement(GNet *net);
  };
} // namespace eda::gate::tech_optimizer
