//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/cut_storage.h"
#include "gate/optimizer/util.h"
#include "gate/optimizer/visitor.h"

#include <deque>

namespace eda::gate::optimizer {

 /**
  * \brief Finds cone for given node and its cut.
  */
  class ConeVisitor : public Visitor {

  public:
    using Gate = eda::gate::model::Gate;
    using GateSymbol = eda::gate::model::GateSymbol;

    /**
     * @param cut Set of nodes on base of which cone needs to be found.
     * @param cutFor Node for which cone needs to be found.
     */
    ConeVisitor(const Cut &cut, GateId cutFor);

    VisitorFlags onNodeBegin(const GateId &) override;

    VisitorFlags onNodeEnd(const GateId &) override;

    /**
     * @return Found cone net.
     */
    GNet *getGNet();

    /**
     * @return Maps inputs in original net with inputs of created cone net.
     */
    const MatchMap &getResultMatch();

    /**
     * @return Nonredundant cut for the node for which cone was found.
     */
    const Cut &getResultCutOldGates();

  private:
    const Cut &cut;
    GateId cutFor;
    // old node - new node.
    MatchMap newGates;
    Cut resultCutOldGates;
    GNet *net;

  };

} // namespace eda::gate::optimizer
