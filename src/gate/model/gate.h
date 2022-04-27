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

#include "gate/model/gsymbol.h"
#include "gate/model/signal.h"

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
  // Creation.
  friend class Netlist;
  friend class eda::rtl::compiler::Compiler;

public:
  using Id = unsigned;
  using List = std::vector<Gate *>;

  /// Returns the gate from the storage.
  static Gate* get(Gate::Id id) { return _storage[id]; }

  Id id() const { return _id; }
  GateSymbol kind() const { return _kind; }
  std::size_t arity() const { return _inputs.size(); }

  const Signal::List& inputs() const { return _inputs; }
  const Signal& input(std::size_t i) const { return _inputs[i]; }

  bool is_source() const { return _kind == GateSymbol::NOP && _inputs.empty(); }
  bool is_value() const { return _kind == GateSymbol::ONE || _kind == GateSymbol::ZERO; }
  bool is_trigger() const { return is_sequential(); }
  bool is_gate() const { return !is_source() && !is_trigger(); }

private:
  Gate(GateSymbol gate, const Signal::List inputs):
    _id(next_id()), _kind(gate), _inputs(inputs) {
    // Register the gate in the storage.
    if (_id >= _storage.size()) {
      _storage.resize(_id + 1);
      _storage[_id] = this;
    }
  }

  Gate(): Gate(GateSymbol::NOP, {}) {}

  bool is_sequential() const {
    for (const auto &input: _inputs) {
      if (input.kind() != Event::ALWAYS) {
        return true;
      }
    }
    return false;
  }

  void set_kind(GateSymbol kind) { _kind = kind; }

  void set_inputs(const Signal::List &inputs) {
    _inputs.assign(inputs.begin(), inputs.end());
  }

  const Id _id;

  GateSymbol _kind;
  Signal::List _inputs;

  /// Returns the next gate identifier.
  static Gate::Id next_id() { return _storage.size(); }

  /// Common gate storage
  static Gate::List _storage;
};

std::ostream& operator <<(std::ostream &out, const Gate &gate);

} // namespace eda::gate::model
