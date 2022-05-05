//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gsymbol.h"
#include "gate/model/signal.h"

#include <algorithm>
#include <iostream>
#include <vector>

namespace eda::rtl::compiler {
  class Compiler;
} // namespace eda::rtl::compiler

namespace eda::gate::model {

class GNet;

/**
 * \brief Represents a logic gate or a flip-flop/latch.
 * \author <a href="mailto:kamkin@ispras.ru">Alexander Kamkin</a>
 */
class Gate final {
  // To create gates when synthesizing netlists.
  friend class GNet;
  friend class eda::rtl::compiler::Compiler;

public:
  using Id = Signal::GateId;
  using List = std::vector<Gate*>;
  using Link = std::pair<Id, std::size_t>;
  using LinkList = std::vector<Link>;

  /// Returns the gate w/ the given id from the storage.
  static Gate* get(Gate::Id id) { return _storage[id]; }

  Id id() const { return _id; }
  GateSymbol kind() const { return _kind; }
  std::size_t arity() const { return _inputs.size(); }
  std::size_t fanout() const { return _links.size(); }

  const Signal::List& inputs() const { return _inputs; }
  const Signal& input(std::size_t i) const { return _inputs[i]; }

  const LinkList links() const { return _links; }
  const Link& link(std::size_t i) const { return _links[i]; }

  bool isSource() const {
    return _kind == GateSymbol::NOP && _inputs.empty();
  }

  bool isValue() const {
    return _kind == GateSymbol::ONE || _kind == GateSymbol::ZERO;
  }

  bool isTrigger() const {
    for (const auto &input: _inputs) {
      if (input.kind() != Event::ALWAYS)
        return true;
    }
    return false;
  }

  bool isComb() const {
    return !isSource() && !isTrigger();
  }

private:
  /// Creates a gate w/ the given operation and the inputs.
  Gate(GateSymbol kind, const Signal::List inputs):
    _id(_storage.size()), _kind(kind), _inputs(inputs) {
    // Register the gate in the storage.
    if (_id >= _storage.size()) {
      _storage.resize(_id + 1);
      _storage[_id] = this;
    }
    appendLinks();
  }

  /// Creates a source gate.
  Gate(): Gate(GateSymbol::NOP, {}) {}

  void setKind(GateSymbol kind) {
    _kind = kind;
  }

  void appendLink(const Link &link) {
    _links.push_back(link);
  }

  void removeLink(const Link &link) {
    auto i = std::remove(_links.begin(), _links.end(), link);
    _links.erase(i, _links.end());
  }

  void appendLinks() {
    for (std::size_t i = 0; i < _inputs.size(); i++) {
      auto *gate = Gate::get(_inputs[i].gateId());
      gate->appendLink({ _id, i });
    }
  }

  void removeLinks() {
    for (std::size_t i = 0; i < _inputs.size(); i++) {
      auto *gate = Gate::get(_inputs[i].gateId());
      gate->removeLink({ _id, i });
    }
  }

  void setInputs(const Signal::List &inputs);

  const Id _id;
  GateSymbol _kind;
  Signal::List _inputs;
  LinkList _links;

  /// Common gate storage.
  static Gate::List _storage;
};

std::ostream& operator <<(std::ostream &out, const Gate &gate);

} // namespace eda::gate::model
