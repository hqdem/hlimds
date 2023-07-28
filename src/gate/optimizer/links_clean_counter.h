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
  * \brief Visitor class to count number of gates that would be removed
  * in a net during substitution.
  */
  class LinksRemoveCounter : public Visitor {

  public:
    using Gate = eda::gate::model::Gate;
    using GateSymbol = eda::gate::model::GateSymbol;
    using Signal = base::model::Signal<GNet::GateId>;

    /**
     * @param targetsList List of out node and its predecessor.
     * @param used Set of used gate IDs from the initial net.
     * @param removedOrder List where removed gates are stored
     * in order of their calling.
     */
    LinksRemoveCounter(TargetsList &&targetsList,
                       const std::unordered_set<GateID> &used,
                       std::vector<GateID> &removedOrder);

    LinksRemoveCounter(const TargetsList &targetsList,
                       const std::unordered_set<GateID> &used,
                       std::vector<GateID> &removedOrder);

    VisitorFlags onNodeBegin(const GateID &) override;

    VisitorFlags onNodeEnd(const GateID &) override;

    /**
     * @return Number of nodes that were removed.
     */
    int getNRemoved() {
      // -1 because we don't really remove the target node, that is in the set.
      return static_cast<int>(removed.size()) - static_cast<int>(targets.getTargetsSize());
    }

  private:
    TargetsList targets;
    const std::unordered_set<GateID> &used;
    std::unordered_set<GateID> removed;
    std::vector<GateID> &removedOrder;
  };

} // namespace eda::gate::optimizer

