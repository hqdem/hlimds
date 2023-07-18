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

  bool ExhausitiveSearchOptimizer::checkOptimize(const GateID &lastNode,
                                                 const BoundGNet &option,
                                                 MatchMap &map) {
    netSubstitute = NetSubstitute(lastNode, &map, option.net.get(), net);
    int reduce = netSubstitute.fakeSubstitute();
    if (reduce < bestReduce) {
      bestReduce = reduce;
      return true;
    }
    return false;
  }

  void ExhausitiveSearchOptimizer::considerOptimization(const GateID &lastNode,
                                                        BoundGNet &option,
                                                        std::unordered_map
                                                        <GateID, GateID> &map) {
    bestOption = std::move(netSubstitute);
  }

  BoundGNetList
  ExhausitiveSearchOptimizer::getSubnets(uint64_t func) {
    return rwdb.get(func);
  }

  VisitorFlags ExhausitiveSearchOptimizer::finishOptimization(
          const GateID &lastNode) {
    if (bestReduce <= 0) {
      bestOption.substitute();
    }
    bestReduce = 1;
    return CONTINUE;
  }

} // namespace eda::gate::optimizer
