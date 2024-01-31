//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/miter.h"

namespace eda::gate::debugger {

bool areMiterable(const GNet &net1, const GNet &net2, const Hints &hints) {
  GateBinding *sources = hints.sourceBinding.get();
  GateBinding *targets = hints.targetBinding.get();

  if (sources->empty() &&
     (net1.constants().empty() || net2.constants().empty())) {
    CHECK(false) << "Hints contain 0 sources" << std::endl;
    return false;
  }

  if (targets->empty()) {
    CHECK(false) << "Hints contain 0 targets" << std::endl;
    return false;
  }

  if (net1.nSourceLinks() != net2.nSourceLinks()) {
    CHECK(false) << "Nets do not have the same number of inputs" << std::endl;
    return false;
  }

  for (auto source : net1.sourceLinks()) {
    if (sources->find(source) ==
        sources->end()) {
      CHECK(false) << "Can't find source, id=" << source.target << std::endl;
      return false;
    }
  }
  for (auto target : net1.targetLinks()) {
    if (targets->find(target) ==
        targets->end()) {
      CHECK(false) << "Can't find target, id=" << target.source << std::endl;
      return false;
    }
  }

  return true;
}

GNet *miter(const GNet &net1, const GNet &net2, const GateIdMap &gmap) {
  Hints hints = makeHints(net1, net2, gmap);
  if (not areMiterable(net1, net2, hints)) {
    return nullptr;
  }

  GateIdMap map1 = {};
  GateIdMap map2 = {};
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

  SatChecker::Hints newHints;
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
