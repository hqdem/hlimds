//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/net_substitute.h"
#include "gate/optimizer/optimizer_visitor.h"
#include "gate/optimizer/rwmanager.h"

namespace eda::gate::optimizer {

  /**
   * \brief Replacement is performed right away in place
   * if it improves the criterion of the number of nodes in the net.
   * \author <a href="mailto:dreamer_1977@ispras.ru">Liza Shcherbakova</a>
   */
  class ApplySearchOptimizer : public OptimizerVisitor {

  private:
    NetSubstitute netSubstitute;
    RWDatabase rwdb;

  public:
    ApplySearchOptimizer() {
      RewriteManager rewriteManager;
      rewriteManager.initialize();
      rwdb = rewriteManager.getDatabase();
    }

    bool checkOptimize(const GateID &lastNode, const BoundGNet &option,
                       MatchMap &map) override;

    void considerOptimization(const GateID &lastNode, BoundGNet &option,
                              MatchMap &map) override;

    BoundGNetList getSubnets(uint64_t func) override;
  };

} // namespace eda::gate::optimizer

