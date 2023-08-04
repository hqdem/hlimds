//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/cut_visitor.h"
#include "gate/optimizer/visitor.h"
#include "gate/printer/dot.h"

#include <filesystem>

namespace eda::gate::optimizer {

 /**
  * \brief Visitor class that prints given net on each step.
  */
  class TrackerVisitor : public CutVisitor {

  public:
    /**
     * @param subCatalog Path to the folder for outputting log information.
     * @param net Net that will be traced.
     * @param visitor Implementation of interface OptimizerVisitor.
     * which corresponding methods will be called.
     */
    TrackerVisitor(const std::filesystem::path &subCatalog, const GNet *net,
                   CutVisitor *visitor);

    VisitorFlags onNodeBegin(const GateId &) override;

    VisitorFlags onNodeEnd(const GateId &) override;

    VisitorFlags onCut(const GateId &, const Cut &) override;

  private:
    std::filesystem::path subCatalog;
    CutVisitor *visitor;
    Dot dot;
    int counter = 0;
  };

} // namespace eda::gate::optimizer
