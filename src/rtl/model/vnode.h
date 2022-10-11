//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "base/model/signal.h"
#include "rtl/model/fsymbol.h"
#include "rtl/model/variable.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

namespace eda::rtl::model {

class Net;
class PNode;

/**
 * \brief Represents a V-node (V = variable), a functional or communication unit of the design.
 * \author <a href="mailto:kamkin@ispras.ru">Alexander Kamkin</a>
 */
class VNode final {
  // Creation.
  friend class Net;
  // Setting the parent p-node.
  friend class PNode;

public:
  using List = std::vector<VNode*>;
  using Event = base::model::Signal<VNode*>;
  using EventList = Event::List;

  enum Kind {
    /// Source node (S-node):
    ///   input wire x.
    SRC,
    /// Constant node (C-node):
    ///   y <= (c[0], ..., c[n-1]).
    VAL,
    /// Functional node (F-node):
    ///   always_comb y <= f(x[0], ..., x[n-1]).
    FUN,
    /// Multiplexor node (M-node):
    ///   always_comb y <= mux(x[0], ..., x[n-1]).
    MUX,
    /// Register node (R-node):
    ///   always_ff @(edge) y <= x or always_latch if(level) y <= x.
    REG
  };

  Kind kind() const { return _kind; }

  const Variable &var() const { return _var; }
  const std::string &name() const { return _var.name(); }
  const Type &type() const { return _var.type(); }

  std::size_t esize() const { return _events.size(); }
  const EventList &events() const { return _events; }
  const Event &event(std::size_t i) const { return _events[i]; }

  FuncSymbol func() const { return _func; }
  std::size_t arity() const { return _inputs.size(); }

  const List &inputs() const { return _inputs; }
  const VNode *input(std::size_t i) const { return _inputs[i]; }
  VNode *input(std::size_t i) { return _inputs[i]; }

  const std::vector<bool> value() const { return _value; }

  const PNode *pnode() const { return _pnode; }

private:
  VNode(Kind kind,
        const Variable &var,
        const EventList &events,
        FuncSymbol func,
        const List &inputs,
        const std::vector<bool> &value):
      _kind(kind),
      _var(var),
      _events(events),
      _func(func),
      _inputs(inputs),
      _value(value),
      _pnode(nullptr) {
    assert(std::find(inputs.begin(), inputs.end(), nullptr) == inputs.end());
  }

  VNode *duplicate(const std::string &new_name) {
    Variable var(new_name, _var.kind(), _var.bind(), _var.type());
    return new VNode(_kind, var, _events, _func, _inputs, _value);
  }

  void replace_with(Kind kind,
                    const Variable &var,
                    const EventList &events,
                    FuncSymbol func,
                    const List &inputs,
                    const std::vector<bool> &value) {
    this->~VNode();
    new (this) VNode(kind, var, events, func, inputs, value);
  }

  void set_pnode(const PNode *pnode) {
    assert(pnode != nullptr);
    _pnode = pnode;
  }

  const Kind _kind;
  const Variable _var;
  const EventList _events;
  const FuncSymbol _func;
  const List _inputs;
  const std::vector<bool> _value;

  // Parent p-node (set on p-node creation).
  const PNode *_pnode;
};

std::ostream& operator <<(std::ostream &out, const VNode &vnode);

} // namespace eda::rtl::model
