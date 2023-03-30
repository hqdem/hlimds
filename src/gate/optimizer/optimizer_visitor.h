//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/cuts_finder_visitor.h"
#include "gate/optimizer/links_clean.h"
#include "gate/optimizer/util.h"
#include "gate/optimizer/visitor.h"
#include "gate/simulator/simulator.h"

#include <queue>

namespace eda::gate::optimizer {
/**
 * \brief Realization of interface Visitor.
 * \ Handler of the node and its cut to execute rewriting.
 * \author <a href="mailto:dreamer_1977@ispras.ru">Liza Shcherbakova</a>
 */
  class OptimizerVisitor : public Visitor {
  public:

    OptimizerVisitor(CutStorage *cutStorage, GNet *net);

    VisitorFlags onNodeBegin(const GateID &) override;

    VisitorFlags onNodeEnd(const GateID &) override;

    VisitorFlags onCut(const Cut &) override;

  private:
    CutStorage *cutStorage;
    GNet *net;
    GNet::V lastNode;
    CutStorage::Cuts *lastCuts;
    std::vector<const CutStorage::Cut *> toRemove;

    GNet *getSubnet(uint64_t func);
  };

} // namespace eda::gate::optimizer