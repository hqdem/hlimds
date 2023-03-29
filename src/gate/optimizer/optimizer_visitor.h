//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/links_clean.h"
#include "gate/optimizer/visitor.h"
#include "gate/optimizer/walker.h"
#include "gate/simulator/simulator.h"
#include "gate/optimizer/util.h"

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

    GNet *getSubnet(uint64_t func);

    // TODO: move to separate file.
  public:

    bool
    fakeSubstitute(GateID cutFor, const Cut &cut, GNet *subsNet, GNet *net);

    uint64_t getTruthTable(GateID cutFor, const Cut &cut, GNet *net);
  };

} // namespace eda::gate::optimizer