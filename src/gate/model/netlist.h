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
  using GateIdList = std::vector<unsigned>;
  using Value = std::vector<bool>;
  using In = std::vector<GateIdList>;
  using Out = GateIdList;
  using ControlEvent = std::pair<Event::Kind, unsigned>;
  using ControlList = std::vector<ControlEvent>;

  Netlist() {
    _gates.reserve(1024*1024);
  } 

  std::size_t size() const { return _gates.size(); }
  const Gate::List& gates() const { return _gates; }
  Gate* gate(std::size_t i) const { return _gates[i]; }

  const GateIdList& sources() const { return _sources; }
  const GateIdList& triggers() const { return _triggers; }

  Signal posedge(unsigned id) const { return Signal(Event::POSEDGE, gate(id)); }
  Signal negedge(unsigned id) const { return Signal(Event::NEGEDGE, gate(id)); }
  Signal level0(unsigned id) const { return Signal(Event::LEVEL0, gate(id)); }
  Signal level1(unsigned id) const { return Signal(Event::LEVEL1, gate(id)); }
  Signal always(unsigned id) const { return Signal(Event::ALWAYS, gate(id)); }

  /// Returns the next gate identifier.
  unsigned next_gate_id() const { return _gates.size(); }

  /// Adds a new source and returns its identifier.
  unsigned add_gate() {
    return add_gate(new Gate(next_gate_id()));
  }

  /// Adds a new gate and returns its identifier.
  unsigned add_gate(GateSymbol kind, const Signal::List &inputs) {
    return add_gate(new Gate(next_gate_id(), kind, inputs));
  }

  /// Modifies the existing gate.
  void set_gate(unsigned id, GateSymbol kind, const Signal::List &inputs);

private:
  unsigned add_gate(Gate *gate) {
    _gates.push_back(gate);

    if (gate->is_source()) {
      _sources.push_back(gate->id());
    } else if (gate->is_trigger()) {
      _triggers.push_back(gate->id());
    }

    return gate->id();
  }

  Gate::List _gates;

  GateIdList _sources;
  GateIdList _triggers;
};

std::ostream& operator <<(std::ostream &out, const Netlist &netlist);

} // namespace eda::gate::model
