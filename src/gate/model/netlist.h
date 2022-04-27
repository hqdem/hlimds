//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gate.h"
#include "rtl/model/event.h"

#include <iostream>
#include <utility>
#include <vector>

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

  Netlist() { _gates.reserve(1024*1024); } 

  std::size_t size() const { return _gates.size(); }
  const Gate::List& gates() const { return _gates; }

  const GateIdList& sources() const { return _sources; }
  const GateIdList& triggers() const { return _triggers; }

  /// Adds a new source and returns its identifier.
  Gate::Id addGate() {
    return addGate(new Gate());
  }

  /// Adds a new gate and returns its identifier.
  Gate::Id addGate(GateSymbol kind, const Signal::List &inputs) {
    return addGate(new Gate(kind, inputs));
  }

  /// Modifies the existing gate.
  void setGate(Gate::Id id, GateSymbol kind, const Signal::List &inputs);

private:
  Gate::Id addGate(Gate *gate) {
    _gates.push_back(gate);

    if (gate->isSource()) {
      _sources.push_back(gate->id());
    } else if (gate->isTrigger()) {
      _triggers.push_back(gate->id());
    }

    return gate->id();
  }

  /// Pointers to the common storage (see below).
  Gate::List _gates;

  GateIdList _sources;
  GateIdList _triggers;
};

std::ostream& operator <<(std::ostream &out, const Netlist &netlist);

} // namespace eda::gate::model
