//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/targets_list.h"

namespace eda::gate::optimizer {

  void TargetsList::defineTargetLinks(GateID gateId) {
    targets.emplace_back(gateId);

    Gate *gate = Gate::get(gateId);

    if (gate->isTarget()) {
      const auto &inputs = gate->inputs();
      assert(inputs.size() == 1);
      targets.emplace_back(inputs[0].node());
    }
  }

  void TargetsList::defineTargetLinks(const TargetsList::GNet *substNet) {
    // Defining target links.
    auto targetLinks = substNet->targetLinks();

    assert(targetLinks.size() == 1 && "SubsNet needs to have 1 output gate.");

    Gate *target = Gate::get(targetLinks.begin()->target);
    assert(target->isTarget());

    targets.emplace_back(target->id());

    // Pushing functional out.
    auto inputs = target->inputs();
    assert(inputs.size() == 1);

    targets.emplace_back(inputs[0].node());
  }

  TargetsList::TargetsList(const GNet *substNet) {
    defineTargetLinks(substNet);
  }

  TargetsList::TargetsList(GateID cutFor) {
    defineTargetLinks(cutFor);
  }

  bool TargetsList::checkOutGate(const Gate *gate) const {
    for (size_t i = 0; i < targets.size(); ++i) {
      if (targets[i] == gate->id()) {
        return true;
      }
    }
    return false;
  }

  size_t TargetsList::getTargetsSize() const {
    return targets.size();
  }

  const std::vector<TargetsList::GateID> &TargetsList::getTargets() const {
    return targets;
  }

} // namespace eda::gate::optimizer