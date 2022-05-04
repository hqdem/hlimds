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

#include <cassert>
#include <iostream>
#include <unordered_map>
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

  #pragma pack(push,1)
  struct GateFlags {
    unsigned source : 1;  // If the gate is output
    unsigned target : 1;  // If the gate is input
    unsigned subnet : 30; // Subnet index plus one
  };
  #pragma pack(pop)

public:
  using List = std::vector<Netlist*>;
  using GateIdList = std::vector<Gate::Id>;
  using Value = std::vector<bool>;
  using In = std::vector<GateIdList>;
  using Out = GateIdList;

  Netlist() {
    const std::size_t N = 1024*1024;

    _gates.reserve(N);
    _flags.reserve(N);
  } 

  std::size_t size() const { return _gates.size(); }
  const Gate::List& gates() const { return _gates; }

  bool isSource(Gate::Id id) const { return getFlags(id).source; }
  bool isTarget(Gate::Id id) const { return getFlags(id).target; }

  void setSource(Gate::Id id, bool value) { getFlags(id).source = value; }
  void setTarget(Gate::Id id, bool value) { getFlags(id).target = value; }

  /// Adds a new source and returns its identifier.
  Gate::Id addGate() {
    return addGate(new Gate(), {});
  }

  /// Adds a new gate and returns its identifier.
  Gate::Id addGate(GateSymbol kind, const Signal::List &inputs) {
    return addGate(new Gate(kind, inputs), {});
  }

  /// Modifies the existing gate.
  void setGate(Gate::Id id, GateSymbol kind, const Signal::List &inputs) {
    auto *gate = Gate::get(id);
    gate->setKind(kind);
    gate->setInputs(inputs);
  }

  /// Returns the gate flags.
  GateFlags getFlags(Gate::Id id) const {
    return _flags.find(id)->second;
  }

  /// Returns the gate flags.
  GateFlags& getFlags(Gate::Id id) {
    return _flags.find(id)->second;
  }

private:
  Gate::Id addGate(Gate *gate, GateFlags flags);
  void addSubnet(Netlist *subnet);

  /// Gates.
  Gate::List _gates;
  /// Flags.
  std::unordered_map<Gate::Id, GateFlags> _flags;

  /// Hierarchy.
  List _subnets;
};

std::ostream& operator <<(std::ostream &out, const Netlist &netlist);

} // namespace eda::gate::model
