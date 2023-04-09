//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "zero_optimizer.h"

namespace eda::gate::optimizer {
  using BoundGNetList = RWDatabase::BoundGNetList;

  bool ZeroOptimizer::checkOptimize(const BoundGNet &,
                                    const std::unordered_map<GateID, GateID> &map) {
    return true;
  }

  VisitorFlags
  ZeroOptimizer::considerOptimization(BoundGNet &option,
                                      std::unordered_map<GateID, GateID> &map) {
    substitute(lastNode, map, option.net.get(), net);
    return FINISH_THIS;
  }

  // TODO: correct method.
  BoundGNetList ZeroOptimizer::getSubnets(uint64_t func) {
    BoundGNet rez;
    GNet *subsNet = new GNet();
    rez.net = std::shared_ptr<GNet>(subsNet);
    GateID sourceNode = subsNet->addGate(model::GateSymbol::IN);
    rez.bindings.emplace(1, sourceNode);
    subsNet->addGate(model::GateSymbol::OUT,
                     base::model::Signal{
                             base::model::Event::ALWAYS,
                             sourceNode});
    return {rez};
  }
} // namespace eda::gate::optimizer
