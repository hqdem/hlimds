//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/optimizer_visitor.h"
#include "gate/optimizer/rwmanager.h"

namespace eda::gate::optimizer {

  class ExhausitiveSearchOptimizer : public OptimizerVisitor {
  public:
    ExhausitiveSearchOptimizer() {
      RewriteManager rewriteManager;
      rewriteManager.initialize();
      rwdb = rewriteManager.getDatabase();
    }
  protected:
    RWDatabase rwdb;
    bool checkOptimize(const BoundGNet &option,
                       const std::unordered_map<GateID, GateID> &map) override;

    VisitorFlags considerOptimization(const BoundGNet &option,
                                      const std::unordered_map<GateID, GateID> &map) override;

    BoundGNetList getSubnets(uint64_t func) override;
  };
} // namespace eda::gate::optimizer

