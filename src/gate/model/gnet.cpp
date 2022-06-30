//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gnet.h"
#include "util/set.h"

#include <algorithm>

using namespace eda::utils;

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Constructors/Destructors 
//===----------------------------------------------------------------------===//

GNet::GNet(unsigned level):
    _level(level), _nTriggers(0), _nGatesInSubnets(0) {
  const std::size_t N = std::max(1024*1024 >> (5*level), 64);
  const std::size_t M = std::max(1024 >> level, 64);

  _gates.reserve(N);
  _flags.reserve(N);

  _sources.reserve(M);
  _targets.reserve(M);

  _subnets.reserve(M);
}

//===----------------------------------------------------------------------===//
// Gates 
//===----------------------------------------------------------------------===//

GNet::GateId GNet::addGate(Gate *gate, SubnetId sid) {
  const auto gid = gate->id();
  assert(_flags.find(gid) == _flags.end());

  unsigned gindex = _gates.size();
  _gates.push_back(gate);

  GateFlags flags{0, sid, gindex};
  _flags.insert({gid, flags});

  if (checkIfSource(gid)) _sources.insert(gid);
  if (checkIfTarget(gid)) _targets.insert(gid);

  // Modifies the target flags of the in-gates.
  for (auto signal : gate->inputs()) {
    const auto in = signal.gateId();

    if (isTarget(in) && !checkIfTarget(in)) {
      _targets.erase(in);
    }
  }

  // Modifies the source flags of the out-gates.
  for (auto link : gate->links()) {
    const auto out = link.first;

    if (isSource(out) && !checkIfSource(out)) {
      _sources.erase(out);
    }
  }

  if (gate->isTrigger()) {
    _nTriggers++;
  }

  return gate->id();
}

void GNet::setGate(GateId gid, GateSymbol kind, const Signal::List &inputs) {
  auto *gate = Gate::get(gid);

  if (gate->isTrigger()) _nTriggers--;
  gate->setKind(kind);
  gate->setInputs(inputs);
  if (gate->isTrigger()) _nTriggers++;

  // Update the source flag (the target flag does not change).
  if (checkIfSource(gid)) {
    _sources.insert(gid);
  } else {
    _sources.erase(gid);
  }  
}

void GNet::removeGate(GateId gid) {
  auto i = _flags.find(gid);
  assert(i != _flags.end());

  if (_gates.size() == 1) {
    clear();
    return;
  }

  const auto &flags = i->second;

  // If the net is hierarchical, do it recursively.
  if (flags.subnet != INV_SUBNET) {
    auto *subnet = _subnets[flags.subnet];

    subnet->removeGate(gid);
    _nGatesInSubnets--;

    if (subnet->isEmpty()) {
      _emptySubnets.insert(flags.subnet);
    }
  }

  auto *gate = Gate::get(gid); 
  auto *last = _gates.back();

  _gates[flags.gindex] = last;
  _gates.erase(std::end(_gates) - 1, std::end(_gates));
  getFlags(last->id()).gindex = flags.gindex;

  _sources.erase(gid);
  _targets.erase(gid);

  // Modifies the target flags of the in-gates.
  for (auto signal : gate->inputs()) {
    const auto in = signal.gateId();

    if (!isTarget(in) && checkIfTarget(in)) {
      _targets.insert(in);
    }
  }

  // Modifies the source flags of the out-gates.
  for (auto link : gate->links()) {
    const auto out = link.first;

    if (!isSource(out) && checkIfSource(out)) {
      _sources.insert(out);
    }
  }

  if (gate->isTrigger()) {
    _nTriggers--;
  }

  _flags.erase(i);
}

bool GNet::checkIfSource(GateId gid) const {
  // ASSERT: the gate is in this net.
  const auto *gate = Gate::get(gid);

  // Real source.
  if (gate->inputs().empty())
    return true;

  for (auto signal : gate->inputs()) {
    // There is an external in-gate.
    if (!contains(signal.gateId()))
      return true;
  }

  // All in-gates are inside the net.
  return false;
}

bool GNet::checkIfTarget(GateId gid) const {
  // ASSERT: the gate is in this net.
  const auto *gate = Gate::get(gid);

  // Real target.
  if (gate->links().empty())
    return true;

  for (auto link : gate->links()) {
    // There is an external out-gate.
    if (!contains(link.first))
      return true;
  }

  // All out-gates are inside the net.
  return false;
}

//===----------------------------------------------------------------------===//
// Subnets 
//===----------------------------------------------------------------------===//

GNet::SubnetId GNet::newSubnet() {
  if (!_emptySubnets.empty())
    return *_emptySubnets.begin();

  return addSubnet(new GNet(_level + 1));
}

void GNet::addNet(const GNet &net) {
  // ASSERT: this net and the given one are disjoint.

  const auto nG = _gates.size();
  const auto nS = _subnets.size();

  _gates.insert(std::end(_gates),
    std::begin(net._gates), std::end(net._gates));
  _subnets.insert(std::end(_subnets),
    std::begin(net._subnets), std::end(net._subnets));
  _emptySubnets.insert(
    std::begin(net._emptySubnets), std::end(net._emptySubnets));

  _nGatesInSubnets += net._nGatesInSubnets;

  _sources.insert(std::begin(net._sources), std::end(net._sources));
  discard_if(_sources, [this](GateId gid) { return !checkIfSource(gid); });

  _targets.insert(std::begin(net._targets), std::end(net._targets));
  discard_if(_targets, [this](GateId gid) { return !checkIfTarget(gid); });

  for (const auto &[gid, flags] : net._flags) {
    auto newFlags = flags;
    newFlags.subnet += nG;
    newFlags.gindex += nS;

    _flags.insert({gid, newFlags});
  }
}

GNet::SubnetId GNet::addSubnet(GNet *subnet) {
  // ASSERT: this net and the subnet are disjoint.
  assert(subnet);

  SubnetId sid = _subnets.size();
  _subnets.push_back(subnet);

  if (subnet->isEmpty()) {
    _emptySubnets.insert(sid);
  } else {
    for (auto *gate : subnet->gates()) {
      addGate(gate, sid);
    }
  }

  return sid;
}

void GNet::moveGate(GateId gid, SubnetId dst) {
  assert(dst == INV_SUBNET || dst < _subnets.size());

  const auto i = _flags.find(gid);
  assert(i != _flags.end());

  const auto src = i->second.subnet;
  assert(src == INV_SUBNET || src < _subnets.size());

  if (src == dst) 
    return;

  if (src != INV_SUBNET) {
    auto *subnet = _subnets[src];

    subnet->removeGate(gid);
    _nGatesInSubnets--;

    if (subnet->isEmpty()) {
      _emptySubnets.insert(src);
    }
  }

  if (dst != INV_SUBNET) {
    auto *subnet = _subnets[dst];

    subnet->addGate(Gate::get(gid));
    _nGatesInSubnets++;
  }

  i->second.subnet = dst;
}

GNet::SubnetId GNet::mergeSubnets(SubnetId lhs, SubnetId rhs) {
  assert(lhs != INV_SUBNET && lhs < _subnets.size());
  assert(rhs != INV_SUBNET && rhs < _subnets.size());

  if (lhs == rhs)
    return lhs;

  auto *lhsSubnet = _subnets[lhs];
  auto *rhsSubnet = _subnets[rhs];

  // Do merging: lhs = merge(lhs, rhs).
  for (const auto *gate : rhsSubnet->gates()) {
    getFlags(gate->id()).subnet = lhs;
  }

  lhsSubnet->addNet(*rhsSubnet);
  rhsSubnet->clear();

  // Empty subnets are removed by request.
  _emptySubnets.insert(rhs);

  return lhs;
}

GNet::SubnetId GNet::groupOrphans() {
  assert(!isFlat() && hasOrphans());

  const auto sid = newSubnet();
  auto *subnet = _subnets[sid];

  // This is a slow operation.
  for (auto *gate : _gates) {
    GateFlags &flags = getFlags(gate->id());

    if (flags.subnet == INV_SUBNET) {
      flags.subnet = sid;
      subnet->addGate(gate);

      _nGatesInSubnets++;
      if (_nGatesInSubnets == _gates.size())
        break;
    }
  }

  assert(_nGatesInSubnets == _gates.size());
  return sid;
}

void GNet::flatten() {
  for (auto &[gid, flags] : _flags) {
    flags.subnet = INV_SUBNET;
  }

  _subnets.clear();
  _emptySubnets.clear();
  _nGatesInSubnets = 0;
}

void GNet::removeEmptySubnets() {
  if (_emptySubnets.empty())
    return;

  // ASSUME: indices of the empty subnets are sorted.
  unsigned nRemoved = 0;

  auto removeIt = _emptySubnets.begin();
  auto removeId = *removeIt;
  for (auto i = removeId; i < _subnets.size(); i++) {
    if (i == removeId) {
      nRemoved++;
      removeIt++;
      removeId = (removeIt == _emptySubnets.end()) ? INV_SUBNET : *removeIt;
      continue;
    }

    // Modifies the subnet index.
    auto *subnet = _subnets[i];
    for (const auto *gate : subnet->gates()) {
      getFlags(gate->id()).subnet -= nRemoved;
    }
 
    // Shift the subnets.
    _subnets[i - nRemoved] = subnet;
  }

  // Remove the empty subnets.
  _subnets.erase(std::end(_subnets) - nRemoved, std::end(_subnets));
  _emptySubnets.clear();
}

void GNet::clear() {
  _gates.clear();
  _flags.clear();
  _sources.clear();
  _targets.clear();
  _subnets.clear();
  _emptySubnets.clear();
  _nGatesInSubnets = 0;
}

//===----------------------------------------------------------------------===//
// Output 
//===----------------------------------------------------------------------===//

std::ostream& operator <<(std::ostream &out, const GNet &net) {
  for (const auto *gate: net.gates()) {
    out << *gate << std::endl;
  }
  return out;
}
 
} // namespace eda::gate::model
