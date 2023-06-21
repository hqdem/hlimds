//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/rwmanager.h"
#include "gate/tech_mapper/tech_map_visitor.h"

#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>

namespace eda::gate::optimizer {

  class SimpleTechMapper : public TechMapVisitor {
  public:
    SimpleTechMapper() {}

  protected:

    BoundGNet bestOption;
    std::unordered_map<GateID, GateID> bestOptionMap;

    bool checkOptimize(const BoundGNet &superGate,
        const std::unordered_map<GateID, GateID> &map) override;

    VisitorFlags considerTechMap(BoundGNet &superGate,
        std::unordered_map<GateID, GateID> &map) override;

    BoundGNetList getSubnets(uint64_t func) override;

    void finishTechMap() override;

    double maxArrivalTime(const BoundGNet &superGate,
        const std::unordered_map<GateID, GateID> &map);
  };
  
} // namespace eda::gate::optimizer