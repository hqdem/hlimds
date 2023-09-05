//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/depth_find_visitor.h"

#include <map>

namespace eda::gate::optimizer {

 /**
  * \brief Visitor class to balance associative operations in the given net.
  */
  class AssocBalanceVisitor : public Visitor {
    friend class AssociativeBalancer;
  public:
    using Gate = model::Gate;
    using GateDMap = DepthFindVisitor::GateDMap;

    AssocBalanceVisitor(GNet *, GateDMap &);

    VisitorFlags onNodeBegin(const GateId &) override;

    VisitorFlags onNodeEnd(const GateId &) override;

  private:
    GNet *net;
    GateDMap &gateDepth;
    int balancesNumber;

    void updateDepth(const GateId &);

    /* Starts balancing operations on the gate. */
    int balanceOnGate(const GateId &);

    /* Moves associative operation to the left/right, */
    /* depending on what type of iterator is provided */
    /* (reverse_iterator or iterator).                */
    template<typename Iter>
    void moveOp(const GateId &, const Iter, const Iter, const Iter, const Iter,
                const Iter);

    /* Moves associative operation to the left/right,   */
    /* depending on what type of iterator is provided,  */
    /* while the depth of upper gate is not increasing. */
    template<typename Iter>
    void moveOpToLim(const GateId &, Iter, const Iter, const Iter, const Iter,
                     const Iter);

    /* Checks if it is possible to balance operations */
    /* using associativity property.                  */
    bool canBalanceAssoc(const GateId &, const GateId &) const;

    /* Checks if it is possible to balance operations */
    /* using complementary associativity property.    */
    bool canBalanceCompl(const GateId &, const GateId &, const GateId &) const;

    /* Checks if it is possible to balance operations      */
    /* using associativity or complementary associativity. */
    bool canBalance(const GateId &, const GateId &, const GateId &) const;

    /* Moves all associative input operations left while */
    /* the depth of the parent gate is not increasing.   */
    void moveAllOpsLToLim(const GateId &);

    /* Moves all associative input operations right while */
    /* the depth of the parent gate is not increasing.    */
    void moveAllOpsRToLim(const GateId &);

    /* Implements associative balancing for operations. */
    void balanceAssoc(const GateId &);

    /* Implements associative balancing for operations, */
    /* that are commutative too.                        */
    void balanceCommutAssoc(const GateId &);

    /* Implements complementary associative balancing. */
    /* In current version it is only MAJ3 function.    */
    void balanceComplAssoc(const GateId &);

    /* Returns total number of depth decreases on each gate. */
    int getBalancesNumber() const;
  };

} // namespace eda::gate::optimizer
