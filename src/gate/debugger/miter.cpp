//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/miter.h"

namespace eda::gate::debugger {

bool areMiterable(GNet &net1, GNet &net2, Hints &hints) {
  if (net1.nSourceLinks() != net2.nSourceLinks()) {
    CHECK(false) << "Nets do not have the same number of inputs\n";
    return false;
  }

  for (auto sourceLink : net1.sourceLinks()) {
    if (hints.sourceBinding.get()->find(sourceLink) ==
        hints.sourceBinding.get()->end()) {
      CHECK(false) << "Unable to find source with id " <<
                      sourceLink.target << '\n';
      return false;
    }
  }
  for (auto targetLink : net1.targetLinks()) {
    if (hints.targetBinding.get()->find(targetLink) ==
        hints.sourceBinding.get()->end()) {
      CHECK(false) << "Unable to find target with id " <<
                      targetLink.source << '\n';
      return false;
    }
  }

  return true;
}

GNet *miter(GNet &net1, GNet &net2, Hints &hints) {
  if (not areMiterable(net1, net2, hints)) {
    return nullptr;
  }

  std::unordered_map<Gate::Id, Gate::Id> map1 = {};
  std::unordered_map<Gate::Id, Gate::Id> map2 = {};
  GNet *cloned1 = net1.clone(map1);
  GNet *cloned2 = net2.clone(map2);

  GateBinding ibind, obind, tbind;
  // Input-to-input correspondence.
  for (auto bind : *hints.sourceBinding.get()) {
    auto newSourceId1 = map1[bind.first.target];
    auto newSourceId2 = map2[bind.second.target];
    ibind.insert({Gate::Link(newSourceId1), Gate::Link(newSourceId2)});
  }

  // Output-to-output correspondence.
  for (auto bind : *hints.targetBinding.get()) {
    auto newTargetId1 = map1[bind.first.source];
    auto newTargetId2 = map2[bind.second.source];
    obind.insert({Gate::Link(newTargetId1), Gate::Link(newTargetId2)});
  }

  Checker::Hints newHints;
  newHints.sourceBinding  = std::make_shared<GateBinding>(std::move(ibind));
  newHints.targetBinding  = std::make_shared<GateBinding>(std::move(obind));

  SignalList inputs, xorSignalList;
  GNet *miter = new GNet();
  miter->addNet(*cloned1);
  miter->addNet(*cloned2);

  for (auto bind : *newHints.sourceBinding.get()) {
    GateId newInputId = miter->addIn();
    miter->replace(bind.first.target, newInputId);
    miter->replace(bind.second.target, newInputId);
  }

  for (auto bind : *newHints.targetBinding.get()) {
    GateId newOutId = miter->addXor(bind.first.source, bind.second.source);
    xorSignalList.push_back(Signal::always(newOutId));
  }

  GateId finalOutId = miter->addOr(xorSignalList);
  miter->addOut(finalOutId);
  miter->setOr(finalOutId, xorSignalList);

  for (auto bind : *newHints.targetBinding.get()) {
    miter->setNop(bind.first.source, Gate::get(bind.first.source)->inputs());
    miter->setNop(bind.second.source, Gate::get(bind.second.source)->inputs());
  }

  miter->sortTopologically();
  return miter;
}

Compiled makeCompiled(const GNet &miter) {
  assert(miter.nOuts() == 1);
  static simulator::Simulator simulator;
  GNet::In gnetInput(1);
  auto &input = gnetInput[0];

  for (auto srcLink : miter.sourceLinks()) {
    input.push_back(srcLink.target);
  }

  Gate::SignalList inputs;
  GateId outputId = (*miter.targetLinks().begin()).source;
  GNet::LinkList in;

  for (size_t n = 0; n < miter.nSourceLinks(); n++) {
    in.push_back(GNet::Link(input[n]));
  }

  GNet::LinkList out{Gate::Link(outputId)};

  for (auto input : inputs) {
    in.push_back(GNet::Link(input.node()));
  }

  return simulator.compile(miter, in, out);
}

} // namespace eda::gate::debugger
