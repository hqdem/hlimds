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
  * \brief Visitor class to count number of gates that would be added
  * to a net during substitution.
  */
  class LinkAddCounter : public Visitor {
  public:
    using Gate = eda::gate::model::Gate;
    using Signal = base::model::Signal<GNet::GateId>;

    /**
     * @param targets List of out node and its predecessor.
     * @param net Net for which calculations are executed.
     * @param map Maps gates of substitute net with gates of initial gate.
     * @param toCreate List where nodes that must be created are stored.
     * @param used Set where used nodes from initial net should be stored.
     */
    LinkAddCounter(const TargetsList &targets,
                   Visitor::GNet *net,
                   MatchMap &map,
                   std::vector<GateID> &toCreate,
                   std::unordered_set<GateID> &used);

    VisitorFlags onNodeBegin(const GateID &) override;

    VisitorFlags onNodeEnd(const GateID &) override;

    /**
     * @return Set of used gate IDs from the initial net.
     */
    const std::unordered_set<GateID> &getUsedNet();

    /**
     * @return Number of gates that were used.
     */
    int getUsedNumber() const;

  private:
    const TargetsList &targets;
    GNet *net;
    // Gates IDs of substitute net / gate IDs of old gate.
    MatchMap &map;
    std::unordered_set<GateID> &usedSubNet;
    std::unordered_set<GateID> usedNet;
    std::vector<GateID> &toCreate;
  };

} // namespace eda::gate::optimizer
