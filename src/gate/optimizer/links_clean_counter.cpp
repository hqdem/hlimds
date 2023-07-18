//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/links_clean_counter.h"

namespace eda::gate::optimizer {

  LinksRemoveCounter::LinksRemoveCounter(TargetsList &&targets,
                                         const std::unordered_set<GateID> &used,
                                         std::vector<GateID> &removedOrder)
          : targets(targets), used(used), removedOrder(removedOrder) {
    removed.insert(targets.getTargets().begin(), targets.getTargets().end());
  }

  VisitorFlags LinksRemoveCounter::onNodeBegin(const Visitor::GateID &node) {
    //  Cutting off the case when meeting the node for which cut was found.
    if (targets.checkOutGate(Gate::get(node))) {
      return CONTINUE;
    }

    //  If the node is used, no need to check further nodes -
    //  they can't be erased.
    if (used.find(node) != used.end()) {
      return FINISH_FURTHER_NODES;
    }

    //  If all children of the current node were removed,
    //  the node will be removed as well.
    const auto &links = Gate::get(node)->links();
    for (const auto &link: links) {
      if (removed.find(link.target) == removed.end()) {
        return FINISH_FURTHER_NODES;
      }
    }
    removed.emplace(node);
    removedOrder.push_back(node);

    return CONTINUE;
  }

  VisitorFlags LinksRemoveCounter::onNodeEnd(const Visitor::GateID &) {
    return CONTINUE;
  }

} // namespace eda::gate::optimizer