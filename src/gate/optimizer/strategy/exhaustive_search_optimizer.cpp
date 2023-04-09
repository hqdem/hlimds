//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "exhaustive_search_optimizer.h"

namespace eda::gate::optimizer {
  using BoundGNetList = RWDatabase::BoundGNetList;

  bool ExhausitiveSearchOptimizer::checkOptimize(const BoundGNet &option,
                                                 const std::unordered_map<GateID, GateID> &map) {
    return fakeSubstitute(lastNode, map, option.net.get(), net) < 0;
  }

  VisitorFlags
  ExhausitiveSearchOptimizer::considerOptimization(const BoundGNet &option,
                                                   const std::unordered_map<GateID, GateID> &map) {
    substitute(lastNode, map, option.net.get(), net);
    // TODO: we can return finish all here.
    // TODO: we can make list with nets and their profit.
    return SUCCESS;
  }

  BoundGNetList
  ExhausitiveSearchOptimizer::getSubnets(uint64_t func) {
    return rwdb.get(func);
  }
} // namespace eda::gate::optimizer
