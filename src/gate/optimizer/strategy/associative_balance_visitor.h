//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/depth_find_visitor.h"

namespace eda::gate::optimizer {

 /**
  * \brief Visitor class to balance associative operations in the given net.
  */
  class AssociativeBalanceVisitor : public Visitor {
  public:
    using Gate = model::Gate;
    using GateSymbol = model::GateSymbol;
    using GateDMap = DepthFindVisitor::GateDMap;

    AssociativeBalanceVisitor(GNet *, GateDMap &);

    VisitorFlags onNodeBegin(const GateId &) override;

    VisitorFlags onNodeEnd(const GateId &) override;

  private:
    GNet *net;
    GateDMap &gateDepth;

    void updateDepth(const GateId &);

    bool associativeOperations(const GateId &, const GateId &) const;

    bool balancableOnGate(const GateId &);

    void balanceOnGates(const GateId &, const GateId &);
  };

} // namespace eda::gate::optimizer
