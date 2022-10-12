//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "base/model/link.h"
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
 * \brief Represents a V-node (V = variable), which is a functional or
 *        communication unit of the RTL design.
 * \author <a href="mailto:kamkin@ispras.ru">Alexander Kamkin</a>
 */
class VNode final {
  // Creation.
  friend class Net;
  // Setting the parent p-node.
  friend class PNode;

public:
  using List = std::vector<VNode*>;
  using Link = eda::base::model::Link<VNode*>;
  using LinkList = Link::List;
  using Signal = eda::base::model::Signal<VNode*>;
  using SignalList = Signal::List;

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

  size_t nSignals() const { return _signals.size(); }
  const SignalList &signals() const { return _signals; }
  const Signal &signal(size_t i) const { return _signals[i]; }

  FuncSymbol func() const { return _func; }
  size_t arity() const { return _inputs.size(); }

  const List &inputs() const { return _inputs; }
  const VNode *input(size_t i) const { return _inputs[i]; }
  VNode *input(size_t i) { return _inputs[i]; }

  // FIXME:
  size_t fanout() const { return _links.size(); }
  const LinkList &links() const { return _links; }
  const Link &link(size_t i) const { return _links[i]; }

  const std::vector<bool> value() const { return _value; }

  const PNode *pnode() const { return _pnode; }

private:
  VNode(Kind kind,
        const Variable &var,
        const SignalList &signals,
        FuncSymbol func,
        const List &inputs,
        const std::vector<bool> &value):
      _kind(kind),
      _var(var),
      _signals(signals),
      _func(func),
      _inputs(inputs),
      _value(value),
      _pnode(nullptr) {
    assert(std::find(inputs.begin(), inputs.end(), nullptr) == inputs.end());
  }

  VNode *duplicate(const std::string &new_name) {
    Variable var(new_name, _var.kind(), _var.bind(), _var.type());
    return new VNode(_kind, var, _signals, _func, _inputs, _value);
  }

  void replace_with(Kind kind,
                    const Variable &var,
                    const SignalList &signals,
                    FuncSymbol func,
                    const List &inputs,
                    const std::vector<bool> &value) {
    this->~VNode();
    new (this) VNode(kind, var, signals, func, inputs, value);
  }

  void set_pnode(const PNode *pnode) {
    assert(pnode != nullptr);
    _pnode = pnode;
  }

  const Kind _kind;
  const Variable _var;
  const SignalList _signals;
  const FuncSymbol _func;
  const List _inputs;
  const LinkList _links; // FIXME:
  const std::vector<bool> _value;

  // Parent p-node (set on p-node creation).
  const PNode *_pnode;
};

std::ostream& operator <<(std::ostream &out, const VNode &vnode);

} // namespace eda::rtl::model
