//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/optimizer_visitor.h"
#include "gate/printer/dot.h"

#include <filesystem>

namespace eda::gate::optimizer {

  /**
  * \brief Logs each step of given implementation of interface OptimizerVisitor.
  */
  class TrackStrategy : public OptimizerVisitor {

  public:

    /**
     * @param subCatalog Path to the folder for outputting log information.
     * @param visitor Implementation of interface OptimizerVisitor.
     * which corresponding methods will be called.
     */
    TrackStrategy(const std::filesystem::path &subCatalog,
                  OptimizerVisitor *visitor);

    VisitorFlags onNodeBegin(const GateId &) override;

    bool checkOptimize(const GateId &lastNode, const BoundGNet &option,
                       MatchMap &map) override;

    void considerOptimization(const GateId &lastNode, BoundGNet &option,
                              MatchMap &map) override;

    BoundGNetList getSubnets(uint64_t func) override;

    VisitorFlags finishOptimization(const GateId &) override;

  private:
    std::filesystem::path subCatalog;
    OptimizerVisitor *visitor;
    int counter = 0;
  };

} // namespace eda::gate::optimizer

