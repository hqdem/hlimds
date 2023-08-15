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
    friend class AssociativeBalancer;
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
    int balancesNumber;

    void updateDepth(const GateId &);

    bool associative(const GateId &, const GateId &) const;

    bool commutative(const GateId &, const GateId &) const;

    GateId getDeeperInput(const GateId &) const;

    // 1. f1(f2(x, y), z) -> f1(x, f2(y, z))
    // 2. f1(x, f2(y, z)) -> f1(f2(x, y), z)
    bool canReorderOperations(const GateId &) const;

    bool balanceOnGate(const GateId &);

    bool balanceCommutAssoc(const GateId &, const GateId &);

    void reorderOperationsOnGate(const GateId &);

    bool applyReorderStrategy (const GateId &, const GateId &,
                                const GateId &, const GateId &);

    bool balanceAssoc(const GateId &);

    int getBalancesNumber() const;
  };

} // namespace eda::gate::optimizer
