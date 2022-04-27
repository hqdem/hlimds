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

#include <iostream>
#include <vector>

namespace eda::rtl::compiler {
  class Compiler;
} // namespace eda::rtl::compiler

namespace eda::gate::model {

class Netlist;

/**
 * \brief Represents a logic gate or a flip-flop/latch.
 * \author <a href="mailto:kamkin@ispras.ru">Alexander Kamkin</a>
 */
class Gate final {
  // To create gates when synthesizing netlists.
  friend class Netlist;
  friend class eda::rtl::compiler::Compiler;

public:
  using Id = unsigned;
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
    return isSequential();
  }

  bool isGate() const {
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
    // Update the links.
    for (std::size_t i = 0; i < _inputs.size(); i++) {
      auto *gate = _inputs[i].gate();
      gate->addLink({ _id, i });
    }
  }

  /// Creates a source gate.
  Gate(): Gate(GateSymbol::NOP, {}) {}

  bool isSequential() const {
    for (const auto &input: _inputs) {
      if (input.kind() != Event::ALWAYS) {
        return true;
      }
    }
    return false;
  }

  void setKind(GateSymbol kind) {
    _kind = kind;
  }

  void setInputs(const Signal::List &inputs) {
    _inputs.assign(inputs.begin(), inputs.end());
  }

  void addLink(const Link &link) {
    _links.push_back(link);
  }

  const Id _id;

  GateSymbol _kind;
  Signal::List _inputs;
  LinkList _links;

  /// Common gate storage
  static Gate::List _storage;
};

std::ostream& operator <<(std::ostream &out, const Gate &gate);

} // namespace eda::gate::model
