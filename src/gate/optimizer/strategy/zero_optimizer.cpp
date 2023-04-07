//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "zero_optimizer.h"

namespace eda::gate::optimizer {
  using SwapOption = OptimizerVisitor::SwapOption;

  bool ZeroOptimizer::checkOptimize(const Cut &, const SwapOption &,
                                    const std::unordered_map<GateID, GateID> &map) {
    return true;
  }

  VisitorFlags
  ZeroOptimizer::considerOptimization(const Cut &cut, const SwapOption &option,
                                      const std::unordered_map<GateID, GateID> &map) {
    substitute(lastNode, map, option.subsNet, net);
    return FINISH_THIS;
  }

  std::vector<SwapOption> ZeroOptimizer::getSubnets(uint64_t func) {
    GNet *subsNet = new GNet();
    std::vector<SwapOption> rez;

    std::vector<GateID> order = {subsNet->addGate(model::GateSymbol::IN)};
    order.emplace_back(subsNet->addGate(model::GateSymbol::OUT,
                                        base::model::Signal{
                                                base::model::Event::ALWAYS,
                                                order.back()}));
    rez.emplace_back(subsNet, order);
    return rez;
  }
} // namespace eda::gate::optimizer
