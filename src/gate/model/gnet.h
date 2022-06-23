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
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace eda::rtl::compiler {
  class Compiler;
} // namespace eda::rtl::compiler

namespace eda::gate::model {

/**
 * \brief Represents a gate-level net.
 * \author <a href="mailto:kamkin@ispras.ru">Alexander Kamkin</a>
 */
class GNet final {
  friend class eda::rtl::compiler::Compiler;

public:
  using List = std::vector<GNet*>;
  using GateId = Gate::Id;
  using GateIdList = std::vector<GateId>;
  using SubnetId = unsigned;
  using SubnetIdSet = std::set<SubnetId>;
  using Value = std::vector<bool>;
  using In = std::vector<GateIdList>;
  using Out = GateIdList;

  /// Invalid subnet index.
  static const unsigned INV_SUBNET = (1u << 20) - 1;
  /// Maximum subnet index.
  static const unsigned MAX_SUBNET = INV_SUBNET - 1;

  #pragma pack(push,1)
  struct GateFlags {
    /// Shows if the gate is a subnet input.
    unsigned source : 1;
    /// Shows if the gate is a subnet output.
    unsigned target : 1;
    /// Local index of the gate subnet.
    unsigned subnet : 30;
    /// Local index of the gate in this net.
    unsigned gindex : 32;
  };
  #pragma pack(pop)

  GNet() {
    const std::size_t M = 1024;
    const std::size_t N = M*M;

    _gates.reserve(N);
    _flags.reserve(N);

    _subnets.reserve(M);
  }

  /// Checks whether the net is flat.
  bool isFlat() const {
    return _subnets.empty();
  }

  /// Checks whether the net is well-structured.
  bool isGood() const {
    // The net is flat.
    if (_subnets.empty())
      return true;
    // All gates are partitioned into subnets
    return _nGatesInSubnets == _gates.size()
    // and there are no empty subnets.
        && _emptySubnets.empty();
  }

  /// Checks whether the net is empty.
  bool isEmpty() const {
    return _gates.empty();
  }

  /// Returns the number of gates.
  std::size_t nGates() const {
    return _gates.size();
  }

  /// Returns the collection of gates.
  const Gate::List &gates() const {
    return _gates;
  }

  /// Gets a gate by index.
  const Gate *gate(std::size_t index) const {
    return _gates[index];
  }

  /// Adds a new empty gate (source) and returns its identifier.
  GateId newGate() {
    return addGate(new Gate(), {});
  }

  /// Adds a new gate and returns its identifier.
  GateId addGate(GateSymbol kind, const Signal::List &inputs) {
    return addGate(new Gate(kind, inputs), {});
  }

  /// Modifies the existing gate.
  void setGate(GateId id, GateSymbol kind, const Signal::List &inputs) {
    auto *gate = Gate::get(id);
    gate->setKind(kind);
    gate->setInputs(inputs);
  }

  /// Removes the gate from the net.
  void removeGate(GateId id);

  /// Returns a copy of the gate flags.
  GateFlags getFlags(GateId id) const {
    return _flags.find(id)->second;
  }

  /// Returns the reference to the gate flags.
  GateFlags &getFlags(GateId id) {
    return _flags.find(id)->second;
  }

  /// Returns the number of subnets.
  std::size_t nSubnets() const {
    return _subnets.size();
  }

  /// Returns the collection of subnets.
  const List &subnets() const {
    return _subnets;
  }

  /// Gets a subnet by index.
  const GNet *subnet(std::size_t index) const {
    return _subnets[index];
  }

  /// Adds a new empty subnet and returns its identifier.
  SubnetId newSubnet() {
    return addSubnet(new GNet());
  }

  /// Adds the content of the given net.
  void addNet(const GNet *net);

  /// Moves the gate to the given subnet.
  void moveGate(GateId id, SubnetId to);

  /// Merges two subnets and returns the identifier of the joint subnet.
  SubnetId mergeSubnets(SubnetId lhs, SubnetId rhs);

  /// Flattens the net (removes the hierarchy).
  void flatten();

  /// Removes the empty subnets.
  void removeEmptySubnets();

private:
  /// Adds the gate.
  GateId addGate(Gate *gate, GateFlags flags);
  /// Adds the subnet.
  SubnetId addSubnet(GNet *subnet);

  /// Gates of the net.
  Gate::List _gates;

  /// Gate flags.
  std::unordered_map<GateId, GateFlags> _flags;

  /// Input gates of the net.
  std::unordered_set<GateId> _sources;
  /// Output gates of the net.
  std::unordered_set<GateId> _targets;

  /// All subnets including the empty ones.
  List _subnets;
  /// Indices of the empty subnets.
  SubnetIdSet _emptySubnets;

  /// The number of gates that belong to subnets.
  std::size_t _nGatesInSubnets;
};

std::ostream& operator <<(std::ostream &out, const GNet &net);

} // namespace eda::gate::model
