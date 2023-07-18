//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "apply_search_optimizer.h"

namespace eda::gate::optimizer {

  using BoundGNetList = RWDatabase::BoundGNetList;

  bool ApplySearchOptimizer::checkOptimize(const GateID &lastNode, const BoundGNet &option,
                                           MatchMap &map) {
    netSubstitute = NetSubstitute(lastNode, &map, option.net.get(), net);
    return netSubstitute.fakeSubstitute() <= 0;
  }

  void
  ApplySearchOptimizer::considerOptimization(const GateID &lastNode, BoundGNet &option,
                                             MatchMap &map) {
    netSubstitute.substitute();
  }

  BoundGNetList
  ApplySearchOptimizer::getSubnets(uint64_t func) {
    return rwdb.get(func);
  }

} // namespace eda::gate::optimizer
