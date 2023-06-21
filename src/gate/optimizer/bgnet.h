//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gate.h"
#include "gate/model/gnet.h"

#include <map>
#include <memory>
#include <vector>

namespace eda::gate::optimizer {

/**
* \brief GNet with input and output bindings and inputs delay.
* \author <a href="mailto:mrpepelulka@gmail.com">Rustamkhan Ramaldanov</a>
*/
struct BoundGNet {
  using BoundGNetList = std::vector<BoundGNet>;
  using GateBindings = std::vector<model::Gate::Id>;
  using GateId = model::Gate::Id;
  using GateIdMap = model::GNet::GateIdMap;

  std::shared_ptr<model::GNet> net;
  GateBindings inputBindings, outputBindings;
  std::vector<double> inputDelays;
  std::string name;
  double area;

  // Clone with new Gate ID's
  BoundGNet clone() {
    BoundGNet result;

    GateIdMap oldToNewGates;
    result.net = std::shared_ptr<model::GNet>(net->clone(oldToNewGates));

    for (const auto &oldGateId : inputBindings) {
      const auto newGateId = oldToNewGates[oldGateId];
      result.inputBindings.push_back(newGateId);
    }

    for (const auto &oldGateId : outputBindings) {
      const auto newGateId = oldToNewGates[oldGateId];
      result.outputBindings.push_back(newGateId);
    }

    result.inputDelays = inputDelays;

    return result;
  }

};

} // namespace eda::gate::optimizer
