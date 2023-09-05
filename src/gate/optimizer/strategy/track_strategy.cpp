//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/strategy/track_strategy.h"

namespace eda::gate::optimizer {

  TrackStrategy::TrackStrategy(const std::filesystem::path &subCatalog,
                               OptimizerVisitor *visitor) : visitor(visitor) {
    const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
    this->subCatalog = homePath / subCatalog;
  }

  bool TrackStrategy::checkOptimize(const GateId &lastNode, const BoundGNet &option,
                                    MatchMap &map) {

    bool result = visitor->checkOptimize(lastNode, option, map);

    Dot dot(option.net.get());
    dot.print(subCatalog / ("checkOptimize" + std::to_string(counter) + "_" +
                            std::to_string(lastNode) + "_" +
                            std::to_string(result) + ".dot"));
    ++counter;

    return result;
  }

  void TrackStrategy::considerOptimization(const GateId &lastNode,
                                           BoundGNet &option,
                                           MatchMap &map) {
    visitor->considerOptimization(lastNode, option, map);
  }

  OptimizerVisitor::BoundGNetList TrackStrategy::getSubnets(uint64_t func) {
    auto list = visitor->getSubnets(func);
    return list;
  }

  VisitorFlags TrackStrategy::finishOptimization(const GateId &node) {
    return visitor->finishOptimization(node);
  }

  VisitorFlags TrackStrategy::onNodeBegin(const GateId &id) {
    visitor->onNodeBegin(id);
    return OptimizerVisitor::onNodeBegin(id);
  }

} // namespace eda::gate::optimizer
