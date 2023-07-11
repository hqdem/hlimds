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
#include "gate/tech_mapper/replacement_struct.h"
#include "gate/tech_mapper/strategy/strategy.h"

namespace eda::gate::techMap {
  using GNet = eda::gate::model::GNet;
  using GateID = GNet::GateId;
  using CutStorage = eda::gate::optimizer::CutStorage;

  class TechMapper {
  public:
    TechMapper(std::string libertyPath);
    TechMapper(eda::gate::optimizer::SQLiteRWDatabase &rwdb);

    GNet *techMap(GNet *net, Strategy *strategy);
    float getArea(GNet *net);
    float getDelay(GNet *net);

  private:
    //GNet *gNet;
    CutStorage cutStorage;
    std::unordered_map<GateID, double> gatesDelay;
    std::unordered_map<GateID, Replacement> bestReplacement;

    std::string dbPath = "rwtest.db";
    eda::gate::optimizer::SQLiteRWDatabase rwdb;

    double area;
    double delay;

    void aigMap(GNet *net);
    void findCuts(GNet *net);
    void replacementSearch(GNet *net, Strategy *strategy);
    void replacement(GNet *net);
  };
} // namespace eda::gate::techMap