//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/cut_storage.h"
#include "gate/optimizer/visitor.h"

namespace eda::gate::optimizer {

  /**
   * \brief Finds cuts in given net.
   */
  class CutsFindVisitor : public Visitor {

    unsigned int cutSize;
    unsigned int maxCutNum;
    CutStorage *cutStorage;

  public:

    constexpr static unsigned int ALL_CUTS = 0;

    /**
     * @param cutSize Max number of nodes in a cut.
     * @param cutStorage Struct where cuts are stored.
     * @param maxCutsNumber Maximum number of cuts for a single node.
     * To avoid restriction CutsFindVisitor::ALL_CUTS can be used.
     */
    CutsFindVisitor(unsigned int cutSize, CutStorage *cutStorage,
                    unsigned int maxCutsNumber = ALL_CUTS);

    VisitorFlags onNodeBegin(const GateID &) override;

    VisitorFlags onNodeEnd(const GateID &) override;

  };
} // namespace eda::gate::optimizer
