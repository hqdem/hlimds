//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "exhaustive_search_optimizer.h"

namespace eda::gate::optimizer {
  using SwapOption = OptimizerVisitor::SwapOption;

  bool ExhausitiveSearchOptimizer::checkOptimize(const Cut &cut,
                                                 const SwapOption &option,
                                                 const std::unordered_map<GateID, GateID> &map) {
    return fakeSubstitute(lastNode, map, option.subsNet, net) < 0;
  }

  VisitorFlags ExhausitiveSearchOptimizer::considerOptimization(const Cut &cut,
                                                                const SwapOption &option,
                                                                const std::unordered_map<GateID, GateID> &map) {
    substitute(lastNode, map, option.subsNet, net);
    // TODO: we can return finish all here.
    // TODO: we can make list with nets and their profit.
    return SUCCESS;
  }

  std::vector<SwapOption>
  ExhausitiveSearchOptimizer::getSubnets(uint64_t func) {
    return {};
  }
} // namespace eda::gate::optimizer
