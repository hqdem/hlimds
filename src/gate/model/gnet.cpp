//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gnet.h"
#include "util/graph.h"
#include "util/set.h"

#include <algorithm>
#include <cassert>

using namespace eda::utils;
using namespace eda::utils::graph;

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Constructors/Destructors 
//===----------------------------------------------------------------------===//

GNet::GNet(unsigned level):
    _level(level), _nTriggers(0), _nConnects(0), _nGatesInSubnets(0) {
  const std::size_t N = std::max(1024*1024 >> (5*level), 64);
  const std::size_t M = std::max(1024 >> level, 64);

  _gates.reserve(N);
  _flags.reserve(N);

  _sourceLinks.reserve(M);
  _targetLinks.reserve(M);

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

  onAddGate(gate, false);
  return gate->id();
}

void GNet::setGate(GateId gid, GateSymbol kind, const Signal::List &inputs) {
  auto *gate = Gate::get(gid);

  onRemoveGate(gate, true);
  gate->setKind(kind);
  gate->setInputs(inputs);
  onAddGate(gate, true);
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

  onRemoveGate(gate, false);
  _flags.erase(i);
}

void GNet::onAddGate(Gate *gate, bool reconnect) {
  const auto gid = gate->id();

  if (gate->arity() == 0) {
    // If the gate is a pure source, add the link.
    _sourceLinks.insert(Link(gid));
  } else {
    // Add the newly appeared cuts.
    for (std::size_t i = 0; i < gate->arity(); i++) {
      const auto source = gate->input(i).gateId();
      if (!contains(source)) {
        _sourceLinks.insert(Link(source, gid, i));
      }
    }
  }

  // Add the newly appeared cuts.
  for (auto link : gate->links()) {
    const auto target = link.target;
    if (!contains(target)) {
      _targetLinks.insert(link);
    }
  }

  // Remove the previously existing cuts.
  if (!reconnect) {
    for (auto link : gate->links()) {
      _sourceLinks.erase(link);
    }

    for (std::size_t i = 0; i < gate->arity(); i++) {
      const auto source = gate->input(i).gateId();
      _targetLinks.erase(Link(source, gid, i));
    }
  }

  if (gate->isTrigger()) _nTriggers++;
  _nConnects += gate->arity();
}

void GNet::onRemoveGate(Gate *gate, bool reconnect) {
  const auto gid = gate->id();

  if (gate->arity() == 0) {
    // If the gate is a pure source, remove the link.
    _sourceLinks.erase(Link(gid));
  } else {
    // Remove the previously existing cuts.
    for (std::size_t i = 0; i < gate->arity(); i++) {
      const auto source = gate->input(i).gateId();
      _sourceLinks.erase(Link(source, gid, i));
    }
  }

  // Remove the previously existing cuts.
  for (auto link : gate->links()) {
    _targetLinks.erase(link);
  }

  // Add the newly appeared cuts.
  if (!reconnect) {
    for (auto link : gate->links()) {
      const auto target = link.target;
      if (contains(target)) {
        _sourceLinks.insert(link);
      }
    }

    for (std::size_t i = 0; i < gate->arity(); i++) {
      const auto source = gate->input(i).gateId();
      if (contains(source)) {
        _targetLinks.insert(Link(source, gid, i));
      }
    }
  }

  if (gate->isTrigger()) _nTriggers--;
  _nConnects -= gate->arity();
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

  _nTriggers += net._nTriggers;
  _nConnects += net._nConnects;
  _nGatesInSubnets += net._nGatesInSubnets;

  discard_if(_sourceLinks,
    [this](Link link) { return !checkSourceLink(link); });
  for (auto link : net._sourceLinks) {
    if (checkSourceLink(link)) {
      _sourceLinks.insert(link);
    }
  }

  discard_if(_targetLinks,
    [this](Link link) { return !checkTargetLink(link); });
  for (auto link : net._targetLinks) {
    if (checkTargetLink(link)) {
      _targetLinks.insert(link);
    }
  }

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
  _nTriggers = _nConnects = _nGatesInSubnets = 0;

  _gates.clear();
  _flags.clear();
  _sourceLinks.clear();
  _targetLinks.clear();
  _subnets.clear();
  _emptySubnets.clear();
}

//===----------------------------------------------------------------------===//
// Transforms
//===----------------------------------------------------------------------===//

/// Subnet-level graph of the net.
struct Subgraph final {
  using V = GNet*;
  using E = V;

  Subgraph(GNet &net): nV(net.nSubnets()), nE(0) {
    sources.reserve(nV);

    for (auto *subnet : net.subnets()) {
      // Collect sources.
      for (auto sourceLink : subnet->sourceLinks()) {
        if (net.hasSourceLink(sourceLink)) {
          sources.push_back(subnet);
          break;
        }
      }

      // Identify edges.
      auto &outEdges = edges[subnet];
      for (auto targetLink : subnet->targetLinks()) {
        auto *gate = Gate::get(targetLink.source);
        if (gate->isTrigger()) continue;

        for (auto link : gate->links()) {
          auto  sid = net.getSubnetId(link.target);
          auto *end = const_cast<GNet*>(net.subnet(sid));

          if (end != subnet) {
            outEdges.insert(end);
          }
        }
      }

      nE += outEdges.size();
    }
  }

  std::size_t nNodes() const { return nV; }
  std::size_t nEdges() const { return nE; }

  const std::vector<V> &getSources() const {
    return sources;
  }

  const std::unordered_set<E> &getOutEdges(V v) const {
    return edges.find(v)->second;
  }

  V leadsTo(E e) const {
    return e;
  }

  std::size_t nV;
  std::size_t nE;

  std::vector<V> sources;
  std::unordered_map<V, std::unordered_set<E>> edges;
};

void GNet::sortTopologically() {
  assert(isWellFormed());

  // If the net is flat, sort the gates and update the indices.
  if (isFlat()) {
    auto gates = topologicalSort<GNet>(*this);

    for (std::size_t i = 0; i < gates.size(); i++) {
      auto gid = gates[i];

      _gates[i] = Gate::get(gid);
      getFlags(gid).gindex = i;
    }

    return;
  }

  // If the net is hierarchical, sort the subnets.
  Subgraph subgraph(*this);

  using G = Subgraph;
  using E = G::E;
  auto subnets = topologicalSort<G, std::unordered_set<E>>(subgraph);

  // Sort each subnet.
  for (auto *subnet : subnets) {
    subnet->sortTopologically();
  }

  // Sort the gates and update the indices.
  std::size_t offset = 0;
  for (auto *subnet : subnets) {
    for (std::size_t i = 0; i < subnet->nGates(); i++) {
      auto gid = subnet->gate(i)->id();
      auto j = offset + i;

      _gates[j] = Gate::get(gid);
      getFlags(gid).gindex = j;
    }

    offset += subnet->nGates();
  }
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
