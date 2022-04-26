//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <iostream>
#include <utility>
#include <vector>

#include "gate/model/gate.h"
#include "rtl/model/event.h"

namespace eda::rtl::compiler {
  class Compiler;
} // namespace eda::rtl::compiler

namespace eda::gate::model {

/**
 * \brief Represents a gate-level netlist.
 * \author <a href="mailto:kamkin@ispras.ru">Alexander Kamkin</a>
 */
class Netlist final {
  friend class eda::rtl::compiler::Compiler;

public:
  using GateIdList = std::vector<Gate::Id>;
  using Value = std::vector<bool>;
  using In = std::vector<GateIdList>;
  using Out = GateIdList;
  using ControlEvent = std::pair<Event::Kind, Gate::Id>;
  using ControlList = std::vector<ControlEvent>;

  Netlist() {
    _gates.reserve(1024*1024);
  } 

  std::size_t size() const { return _gates.size(); }
  const Gate::List& gates() const { return _gates; }
  Gate* gate(Gate::Id id) const { return _storage[id]; }

  const GateIdList& sources() const { return _sources; }
  const GateIdList& triggers() const { return _triggers; }

  Signal posedge(Gate::Id id) const { return Signal(Event::POSEDGE, gate(id)); }
  Signal negedge(Gate::Id id) const { return Signal(Event::NEGEDGE, gate(id)); }
  Signal level0(Gate::Id id) const { return Signal(Event::LEVEL0, gate(id)); }
  Signal level1(Gate::Id id) const { return Signal(Event::LEVEL1, gate(id)); }
  Signal always(Gate::Id id) const { return Signal(Event::ALWAYS, gate(id)); }

  /// Returns the next gate identifier.
  Gate::Id next_gate_id() const { return _storage.size(); }

  /// Adds a new source and returns its identifier.
  Gate::Id add_gate() {
    return add_gate(new Gate(next_gate_id()));
  }

  /// Adds a new gate and returns its identifier.
  Gate::Id add_gate(GateSymbol kind, const Signal::List &inputs) {
    return add_gate(new Gate(next_gate_id(), kind, inputs));
  }

  /// Modifies the existing gate.
  void set_gate(Gate::Id id, GateSymbol kind, const Signal::List &inputs);

private:
  unsigned add_gate(Gate *gate) {
    _storage.push_back(gate);
    _gates.push_back(gate);

    if (gate->is_source()) {
      _sources.push_back(gate->id());
    } else if (gate->is_trigger()) {
      _triggers.push_back(gate->id());
    }

    return gate->id();
  }

  /// Pointers to the common storage (see below).
  Gate::List _gates;

  GateIdList _sources;
  GateIdList _triggers;

  /// Common gate storage
  static Gate::List _storage;
};

std::ostream& operator <<(std::ostream &out, const Netlist &netlist);

} // namespace eda::gate::model
