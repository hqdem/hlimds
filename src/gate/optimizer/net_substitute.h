//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/links_add_counter.h"
#include "gate/optimizer/links_clean_counter.h"
#include "gate/optimizer/substitute_visitor.h"
#include "gate/optimizer/targets_list.h"
#include "gate/optimizer/util.h"

namespace eda::gate::optimizer {

 /**
  * \brief Class that gradually substitutes part of a net with given subnet.
  * First step - calculation optimization metric for substitution.
  * Second step - executing substitution with the use of data
  * produced during the previous step.
  */
  class NetSubstitute {

  private:
    GateID cutFor;
    GNet *substNet;
    GNet *net;
    Visitor::MatchMap *map;

    std::vector<GateID> toCreate;
    std::vector<GateID> removed;
    std::unordered_set<GateID> used;

    TargetsList targetGates;

  public:
    /**
     * @param cutFor Node for which cone substitution has to be executed.
     * @param map Maps gates of substitute net with gates of initial gate.
     * @param subsNet Net to substitute with.
     * @param net Net where substitution is executed.
     */
    NetSubstitute(GateID cutFor, Visitor::MatchMap *map,
                  GNet *subsNet, GNet *net);

    NetSubstitute();

    /**
     * Performs the first step of substitution -
     * calculating optimization metric.
     * Performs intermediate calculations, which will be useful in the next step
     * @return Value of optimization metric for substitution.
     */
    int fakeSubstitute();

    /**
     * Performs the second step of substitution -
     * executing substitution with the use of data
     * produced during the previous step.
     */
    void substitute();

  };

} // namespace eda::gate::optimizer