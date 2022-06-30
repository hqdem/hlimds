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
  //===--------------------------------------------------------------------===//
  // Types
  //===--------------------------------------------------------------------===//

  using List        = std::vector<GNet*>;
  using GateId      = Gate::Id;
  using GateIdList  = std::vector<GateId>;
  using GateIdSet   = std::unordered_set<GateId>;
  using SubnetId    = unsigned;
  using SubnetIdSet = std::set<SubnetId>;
  using Link        = Gate::Link;
  using LinkList    = Gate::LinkList;
  using Value       = std::vector<bool>;
  using In          = std::vector<GateIdList>;
  using Out         = GateIdList;

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

  //===--------------------------------------------------------------------===//
  // Constants
  //===--------------------------------------------------------------------===//

  /// Invalid subnet index.
  static const unsigned INV_SUBNET = (1u << 20) - 1;
  /// Maximum subnet index (max. 2^20 - 1 subnets).
  static const unsigned MAX_SUBNET = INV_SUBNET - 1;

  //===--------------------------------------------------------------------===//
  // Constructors/Destructors
  //===--------------------------------------------------------------------===//

  /// Constructs an empty net (level=0 stands for the top level).
  explicit GNet(unsigned level = 0);
  /// Destructs the net.
  ~GNet() = default;

  //===--------------------------------------------------------------------===//
  // Properties 
  //===--------------------------------------------------------------------===//

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

  /// Checks whether the net is combinational.
  bool isComb() const {
    return _nTriggers == 0;
  }

  //===--------------------------------------------------------------------===//
  // Statistics 
  //===--------------------------------------------------------------------===//

  /// Returns the number of gates.
  std::size_t nGates() const {
    return _gates.size();
  }

  /// Returns the number of inputs.
  std::size_t nSources() const {
    return _sources.size();
  }

  /// Returns the number of outputs.
  std::size_t nTargets() const {
    return _targets.size();
  }

  /// Returns the number of triggers.
  std::size_t nTriggers() const {
    return _nTriggers;
  }

  /// Returns the number of connections.
  std::size_t nConnects() const {
    return _nConnects;
  }

  /// Returns the number of subnets.
  std::size_t nSubnets() const {
    return _subnets.size();
  }

  //===--------------------------------------------------------------------===//
  // Gates 
  //===--------------------------------------------------------------------===//

  /// Returns the collection of gates.
  const Gate::List &gates() const {
    return _gates;
  }

  /// Returns the collection input gates.
  const GateIdSet &sources() const {
    return _sources;
  }

  /// Returns the collection of output gates.
  const GateIdSet &targets() const {
    return _targets;
  }

  /// Gets a gate by index.
  const Gate *gate(std::size_t index) const {
    return _gates[index];
  }

  /// Checks whether the net contains the gate.
  bool contains(GateId gid) const {
    return _flags.find(gid) != _flags.end();
  }

  /// Checks whether the gate is an input.
  bool isSource(GateId gid) const {
    return _sources.find(gid) != _sources.end();
  }

  /// Checks whether the gate is an output.
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

  //===--------------------------------------------------------------------===//
  // Subnets 
  //===--------------------------------------------------------------------===//

  /// Returns the collection of subnets.
  const List &subnets() const {
    return _subnets;
  }

  /// Gets a subnet by index.
  const GNet *subnet(std::size_t index) const {
    return _subnets[index];
  }

  /// Checks whether the gate is an orphan (does not belong to any subnet).
  bool isOrphan(GateId gid) const {
    return getFlags(gid).subnet == INV_SUBNET;
  }

  /// Returns the subnet the given gate belongs to.
  SubnetId getSubnetId(GateId gid) const {
    return getFlags(gid).subnet;
  }

  /// Adds a new (empty) subnet and returns its identifier.
  SubnetId newSubnet();

  /// Adds the content of the given net.
  void addNet(const GNet &net);

  /// Moves the gate to the given subnet.
  void moveGate(GateId gid, SubnetId dst);

  /// Merges the subnets and returns the identifier of the joint subnet.
  SubnetId mergeSubnets(SubnetId lhs, SubnetId rhs);

  /// Combines all orphan gates into a subnet.
  SubnetId groupOrphans();

  /// Flattens the net (removes the hierarchy).
  void flatten();

  /// Removes the empty subnets.
  void removeEmptySubnets();

  /// Clears the net.
  void clear();

  //===--------------------------------------------------------------------===//
  // Graph Interface
  //===--------------------------------------------------------------------===//

  using V = GateId;
  using E = Link;

  /// Returns the number of nodes.
  std::size_t nNodes() const {
    return nGates();
  }

  /// Returns the number of edges.
  std::size_t nEdges() const {
    return nConnects();
  }

  /// Returns the graph sources.
  const GateIdSet &getSources() const {
    return sources();
  }

  /// Returns the outgoing edges of the node.
  const LinkList &getOutEdges(GateId v) const {
    return Gate::get(v)->links();
  }

  /// Returns the end of the edge.
  GateId leadsTo(const Link &e) const {
    return e.first;
  }

  //===--------------------------------------------------------------------===//
  // Transforms
  //===--------------------------------------------------------------------===//

  /// Sorts the gates in topological order.
  void sortTopologically();

private:
  //===--------------------------------------------------------------------===//
  // Internal Methods
  //===--------------------------------------------------------------------===//

  /// Adds the gate to the net and sets the subnet index.
  /// The subnet is not modified.
  GateId addGate(Gate *gate, SubnetId sid = INV_SUBNET);
  /// Adds the subnet to the net.
  SubnetId addSubnet(GNet *subnet);

  /// Returns a copy of the gate flags.
  GateFlags getFlags(GateId gid) const {
    return _flags.find(gid)->second;
  }

  /// Returns the reference to the gate flags.
  GateFlags &getFlags(GateId gid) {
    return _flags.find(gid)->second;
  }

  /// Checks whether the gate is an input.
  bool checkIfSource(GateId gid) const;
  /// Checks whether the gate is an output.
  bool checkIfTarget(GateId gid) const;

  /// Updates counters when adding a gate.
  void onAddGate(Gate *gate) {
    if (gate->isTrigger()) _nTriggers++;
    _nConnects += gate->arity();
  }

  /// Updates counters when removing a gate.
  void onRemoveGate(Gate *gate) {
    if (gate->isTrigger()) _nTriggers--;
    _nConnects -= gate->arity();
  }

  //===--------------------------------------------------------------------===//
  // Internal Fields
  //===--------------------------------------------------------------------===//

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

  /// Number of triggers.
  std::size_t _nTriggers;
  /// Number of connections.
  std::size_t _nConnects;

  /// All subnets including the empty ones.
  List _subnets;
  /// Indices of the empty subnets.
  SubnetIdSet _emptySubnets;

  /// Number of gates that belong to subnets.
  std::size_t _nGatesInSubnets;
};

/// Outputs the net.
std::ostream& operator <<(std::ostream &out, const GNet &net);

} // namespace eda::gate::model
