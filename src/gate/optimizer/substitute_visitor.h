//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/targets_list.h"
#include "gate/optimizer/visitor.h"

namespace eda::gate::optimizer {

 /**
  * \brief Visitor class to modify given net.
  */
  class SubstituteVisitor : public Visitor {
  public:
    using Gate = model::Gate;

    /**
     * @param targets List of out node and its predecessor.
     * @param cutFor Node for which cone is substituted.
     * @param map Maps cone inputs and substitute net sources.
     * @param net Net where substitution is executed.
     */
    SubstituteVisitor(const TargetsList &targetsList, GateId cutFor,
                      MatchMap &map, GNet *net);

    VisitorFlags onNodeBegin(const GateId &) override;

    VisitorFlags onNodeEnd(const GateId &) override;

  private:
    const TargetsList &targetsList;
    GateId cutFor;
    //  GateId in substitute net -> GateId of original net sources
    MatchMap &map;
    GNet *net;
  };

} // namespace eda::gate::optimizer
