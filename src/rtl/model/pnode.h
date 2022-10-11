//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <iostream>
#include <vector>

#include "rtl/model/event.h"
#include "rtl/model/vnode.h"

namespace eda::rtl::model {

/**
 * \brief Represents a p-node (p = process), a guarded action.
 * \author <a href="mailto:kamkin@ispras.ru">Alexander Kamkin</a>
 */
class PNode final {
  // Creation.
  friend class Net;

public:
  using List = std::vector<PNode *>;

  const Event &event() const { return _event; }

  std::size_t gsize() const { return _guard.size(); }
  const VNode::List &guard() const { return _guard; }
  const VNode *guard(std::size_t i) const { return _guard[i]; }

  std::size_t asize() const { return _action.size(); }
  const VNode::List &action() const { return _action; }
  const VNode* action(std::size_t i) const { return _action[i]; }

private:
  PNode(const Event &event, const VNode::List &guard, const VNode::List &action):
      _event(event), _guard(guard), _action(action) {
    for (auto *vnode: guard) {
      vnode->set_pnode(this);
    }
    for (auto *vnode: action) {
      vnode->set_pnode(this);
    }
  }

  PNode(const VNode::List &guard, const VNode::List &action):
      PNode(Event(), guard, action) {}

  // The execution trigger (posedge, always, etc.).
  const Event _event;
  // The last v-node is the guard bit.
  VNode::List _guard;
  // The non-blocking assignments.
  VNode::List _action;
};

std::ostream &operator <<(std::ostream &out, const PNode &pnode);

} // namespace eda::rtl::model
