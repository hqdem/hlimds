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
 * \brief Represents a (hierarchical) gate-level net.
 * \author <a href="mailto:kamkin@ispras.ru">Alexander Kamkin</a>
 */
class GNet final {
  friend class eda::rtl::compiler::Compiler;

public:
  using List = std::vector<GNet*>;
  using GateId = Gate::Id;
  using GateIdList = std::vector<GateId>;
  using GateIdSet = std::unordered_set<GateId>;
  using SubnetId = unsigned;
  using SubnetIdSet = std::set<SubnetId>;
  using Value = std::vector<bool>;
  using In = std::vector<GateIdList>;
  using Out = GateIdList;

  /// Invalid subnet index.
  static const unsigned INV_SUBNET = (1u << 20) - 1;
  /// Maximum subnet index (max. 2^20 - 1 subnets).
  static const unsigned MAX_SUBNET = INV_SUBNET - 1;

  #pragma pack(push,1)
  struct GateFlags {
    /// Gate flags (reserved).
    unsigned gflags : 12;
    /// Local index of the gate's subnet.
    unsigned subnet : 20;
    /// Local index of the gate.
    unsigned gindex : 32;
  };
  #pragma pack(pop)

  /// Creates an empty net (level=0 stands for the top level).
  GNet(unsigned level = 0): _level(0) {
    const std::size_t N = std::max(1024*1024 >> (5*level), 64);
    const std::size_t M = std::max(1024 >> level, 64);

    _gates.reserve(N);
    _flags.reserve(N);

    _sources.reserve(M);
    _targets.reserve(M);

    _subnets.reserve(M);
  }

  /// Checks whether the net is top-level.
  bool isTop() const {
    return _level == 0;
  }

  /// Checks whether the net is flat.
  bool isFlat() const {
    return _subnets.empty();
  }

  /// Checks whether the net is empty.
  bool isEmpty() const {
    return _gates.empty();
  }

  /// Checks whether the net has orphans.
  bool hasOrphans() const {
    return _nGatesInSubnets < _gates.size();
  }

  /// Checks whether the net has empty subnets.
  bool hasEmptySubnets() const {
    return !_emptySubnets.empty();
  }

  /// Checks whether the net is well-formed.
  bool isWellFormed() const {
    return isFlat() || (!hasOrphans() && !hasEmptySubnets());
  }

  /// Returns the number of gates in the net.
  std::size_t nGates() const {
    return _gates.size();
  }

  /// Returns the collection of the net's gates.
  const Gate::List &gates() const {
    return _gates;
  }

  /// Gets a gate by index.
  const Gate *gate(std::size_t index) const {
    return _gates[index];
  }

  /// Checks whether the net contains the gate.
  bool contains(GateId gid) const {
    return _flags.find(gid) != _flags.end();
  }

  /// Checks whether the gate is source.
  bool isSource(GateId gid) const {
    return _sources.find(gid) != _sources.end();
  }

  /// Checks whether the gate is target.
  bool isTarget(GateId gid) const {
    return _targets.find(gid) != _targets.end();
  }

  /// Adds a new (empty) gate and returns its identifier.
  GateId newGate() {
    return addGate(new Gate());
  }

  /// Adds a new gate and returns its identifier.
  GateId addGate(GateSymbol kind, const Signal::List &inputs) {
    return addGate(new Gate(kind, inputs));
  }

  /// Modifies the existing gate.
  void setGate(GateId gid, GateSymbol kind, const Signal::List &inputs);

  /// Removes the gate from the net.
  void removeGate(GateId gid);

  /// Returns a copy of the gate flags.
  GateFlags getFlags(GateId gid) const {
    return _flags.find(gid)->second;
  }

  /// Returns the reference to the gate flags.
  GateFlags &getFlags(GateId gid) {
    return _flags.find(gid)->second;
  }

  /// Returns the number of subnets in the net.
  std::size_t nSubnets() const {
    return _subnets.size();
  }

  /// Returns the collection of the net's subnets.
  const List &subnets() const {
    return _subnets;
  }

  /// Gets a subnet by index.
  const GNet *subnet(std::size_t index) const {
    return _subnets[index];
  }

  /// Returns the subnet the given gate belongs to.
  SubnetId getGateSubnet(GateId gid) const {
    return getFlags(gid).subnet;
  }

  /// Checks whether the gate is an orphan (does not belong to any subnet).
  bool isOrphan(GateId gid) const {
    return getFlags(gid).subnet == INV_SUBNET;
  }

  /// Adds a new (empty) subnet and returns its identifier.
  SubnetId newSubnet();

  /// Adds the content of the given net.
  void addNet(const GNet &net);

  /// Moves the gate to the given subnet.
  void moveGate(GateId gid, SubnetId dst);

  /// Merges two subnets and returns the identifier of the joint subnet.
  SubnetId mergeSubnets(SubnetId lhs, SubnetId rhs);

  /// Combines all orphan gates into a subnet.
  SubnetId groupOrphans();

  /// Flattens the net (removes the hierarchy).
  void flatten();

  /// Removes the empty subnets.
  void removeEmptySubnets();

  /// Clears the net.
  void clear();

private:
  /// Adds the gate to the net and sets the subnet index.
  /// The subnet is not modified.
  GateId addGate(Gate *gate, SubnetId sid = INV_SUBNET);
  /// Adds the subnet to the net.
  SubnetId addSubnet(GNet *subnet);

  /// Checks whether the gate is source.
  bool checkIfSource(GateId gid) const;
  /// Checks whether the gate is target.
  bool checkIfTarget(GateId gid) const;

  /// Level (0 = top level).
  const unsigned _level;

  /// Gates of the net.
  Gate::List _gates;

  /// Gate flags.
  std::unordered_map<GateId, GateFlags> _flags;

  /// Input gates of the net.
  GateIdSet _sources;
  /// Output gates of the net.
  GateIdSet _targets;

  /// All subnets including the empty ones.
  List _subnets;
  /// Indices of the empty subnets.
  SubnetIdSet _emptySubnets;

  /// The number of gates that belong to subnets.
  std::size_t _nGatesInSubnets;
};

std::ostream& operator <<(std::ostream &out, const GNet &net);

} // namespace eda::gate::model
