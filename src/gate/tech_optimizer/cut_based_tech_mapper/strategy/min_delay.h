//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/rwdatabase.h"
#include "gate/tech_optimizer/cut_based_tech_mapper/strategy/strategy.h"

namespace eda::gate::tech_optimizer {
  using BoundGNet = eda::gate::optimizer::RWDatabase::BoundGNet;
  
/**
 * \author <a href="mailto:dgaryaev@ispras.ru">Daniil Gariaev</a>
 */
  class MinDelay : public Strategy {
  public:
    bool checkOpt(const eda::gate::optimizer::BoundGNet &,
        const eda::gate::model::GNet::GateIdMap &, double &, 
        std::unordered_map<GateID, Replacement> *) override;
  private:
    double maxArrivalTime(const BoundGNet &superGate,
        const std::unordered_map<GateID, GateID> &map,
        std::unordered_map<GateID, Replacement> *bestReplacement);
  };
} // namespace eda::gate::tech_optimizer
