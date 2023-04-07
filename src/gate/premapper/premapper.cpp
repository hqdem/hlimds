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

using Gate = eda::gate::model::Gate;
using GNet = eda::gate::model::GNet;
using SignalList = model::Gate::SignalList;


Gate::SignalList PreMapper::getNewInputs(
    const Gate::SignalList &oldInputs,
    const PreMapper::GateIdMap &oldToNewGates) {
  Gate::SignalList newInputs(oldInputs.size());

  for (size_t i = 0; i < oldInputs.size(); i++) {
    auto oldInput = oldInputs[i];
    auto newInput = oldToNewGates.find(oldInput.node());
    assert("Points one past the last element" &&
            newInput != oldToNewGates.end());

    newInputs[i] = Gate::Signal(oldInput.event(), newInput->second);
  }

  return newInputs;
}

Gate::SignalList PreMapper::getNewInputs(
    const Gate &oldGate,
    const PreMapper::GateIdMap &oldToNewGates,
    size_t &n0,
    size_t &n1) {
  const auto k = oldGate.arity();

  Gate::SignalList newInputs;
  newInputs.reserve(k);

  n0 = 0;
  n1 = 0;
  for (const auto input : oldGate.inputs()) {
    if (model::isValue(input)) {
      const auto isZero = model::isZero(input);
      n0 += (isZero ? 1 : 0);
      n1 += (isZero ? 0 : 1);
    } else {
      const auto i = oldToNewGates.find(input.node());
      assert(i != oldToNewGates.end());

      const auto newInputId = i->second;
      newInputs.push_back(Gate::Signal::always(newInputId));
    }
  }

  return newInputs;
}

std::shared_ptr<GNet> PreMapper::map(const GNet &net,
                                     GateIdMap &oldToNewGates) const {
  auto *newNet = mapGates(net, oldToNewGates);

  // Connect the triggers' inputs.
  for (const auto oldTriggerId : net.triggers()) {
    const auto *oldTrigger = Gate::get(oldTriggerId);

    auto newTriggerId = oldToNewGates.find(oldTriggerId);
    assert("Points one past the last element" &&
            (newTriggerId != oldToNewGates.end()));

    auto newInputs = getNewInputs(oldTrigger->inputs(), oldToNewGates);
    newNet->setGate(newTriggerId->second, oldTrigger->func(), newInputs);
  }

  return std::shared_ptr<GNet>(newNet);
}

GNet *PreMapper::mapGates(const GNet &net,
                          GateIdMap &oldToNewGates) const {
  assert("Orphans, empty subnets, network is not flat or sorted" &&
          (net.isWellFormed() && net.isSorted()));

  auto *newNet = new GNet(net.getLevel());

  if (net.isFlat()) {
    for (const auto *oldGate : net.gates()) {
      const auto oldGateId = oldGate->id();
      assert("Points one past the last element" &&
             (oldToNewGates.find(oldGateId) == oldToNewGates.end()));

      const auto newGateId = mapGate(*oldGate, oldToNewGates, *newNet);
      assert("Invalid gate used" && (newGateId != Gate::INVALID));

      oldToNewGates.emplace(oldGateId, newGateId);
    }

    return newNet;
  }

  for (const auto *oldSubnet : net.subnets()) {
    auto *newSubnet = mapGates(*oldSubnet, oldToNewGates);
    newNet->addSubnet(newSubnet);
  }

  return newNet;
}

Gate::Id PreMapper::mapGate(const Gate &oldGate,
                            const GateIdMap &oldToNewGates,
                            GNet &newNet) const {
  if (oldGate.isSource() || oldGate.isTrigger()) {
    // Triggers' inputs will be connected later.
    return newNet.newGate();
  }

  // Just clone the given gate.
  auto newInputs = getNewInputs(oldGate.inputs(), oldToNewGates);
  return newNet.addGate(oldGate.func(), newInputs);
}

} // namespace eda::gate::premapper
