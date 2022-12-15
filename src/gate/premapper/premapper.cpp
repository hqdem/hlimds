//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/premapper/premapper.h"

#include <cassert>
#include <unordered_map>

namespace eda::gate::premapper {

std::shared_ptr<GNet> PreMapper::map(const GNet &net) const {
  std::unordered_map<Gate::Id, Gate::Id> oldToNewGates;
  oldToNewGates.reserve(net.nGates());

  auto *newNet = map(net, oldToNewGates);
  return std::shared_ptr<GNet>(newNet);
}

GNet *PreMapper::map(const GNet &net, GateIdMap &oldToNewGates) const {
  assert(net.isWellFormed() && net.isSorted());

  auto *newNet = new GNet(net.getLevel());

  if (net.isFlat()) {
    for (const auto *oldGate : net.gates()) {
      const auto oldGateId = oldGate->id();
      assert(oldToNewGates.find(oldGateId) == oldToNewGates.end());

      const auto newGateId = map(*oldGate, oldToNewGates, *newNet);
      assert(newGateId != Gate::INVALID);

      oldToNewGates.insert({oldGateId, newGateId});
    }

    return newNet; 
  }

  for (const auto *oldSubnet : net.subnets()) {
    auto *newSubnet = map(*oldSubnet, oldToNewGates);
    newNet->addSubnet(newSubnet);
  }

  return newNet;
}

Gate::Id PreMapper::map(const Gate &oldGate,
                        const GateIdMap &oldToNewGates,
                        GNet &newNet) const {
  return Gate::INVALID;
}

} // namespace eda::gate::premapper
