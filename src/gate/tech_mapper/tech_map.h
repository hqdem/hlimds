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
#include "gate/tech_mapper/replacement_struct.h"
#include "gate/optimizer/rwdatabase.h"

namespace eda::gate::techMap {
  using GNet = eda::gate::model::GNet;
  using GateID = GNet::GateId;
  using CutStorage = eda::gate::optimizer::CutStorage;

  class TechMap {
  public:
    TechMap(std::string libertyPath);

    GNet *techMap(GNet *net);
    float getArea(GNet *net);
    float getDelay(GNet *net);

  private:
    //GNet *gNet;
    CutStorage cutStorage;
    eda::gate::optimizer::RWDatabase rwdb;
    std::unordered_map<GateID, double> gatesDelay;
    std::unordered_map<GateID, Replacement> bestReplacement;

    float area;
    float delay;

    void aigMap(GNet *net);
    void findCuts(GNet *net);
    void replacementSearch(GNet *net);
    void replacement(GNet *net);
  };
}