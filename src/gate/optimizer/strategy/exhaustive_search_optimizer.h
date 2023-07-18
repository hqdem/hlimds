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
   * \brief Stores the last best found replacement and
   * after the end of the iteration on
   * the list of substitute nets from the database
   * performs the transformation with the best of replacements.
   * \author <a href="mailto:dreamer_1977@ispras.ru">Liza Shcherbakova</a>
   */
  class ExhausitiveSearchOptimizer : public OptimizerVisitor {

  private:
    NetSubstitute netSubstitute;

  public:
    ExhausitiveSearchOptimizer() {
      RewriteManager rewriteManager;
      rewriteManager.initialize();
      rwdb = rewriteManager.getDatabase();
    }

    bool checkOptimize(const GateID &lastNode, const BoundGNet &option,
                       MatchMap &map) override;

    void considerOptimization(const GateID &lastNode, BoundGNet &option,
                              MatchMap &map) override;

    BoundGNetList getSubnets(uint64_t func) override;

    VisitorFlags finishOptimization(const GateID &lastNode) override;

  private:
    RWDatabase rwdb;
    NetSubstitute bestOption;
    int bestReduce = 1;

  };

} // namespace eda::gate::optimizer

