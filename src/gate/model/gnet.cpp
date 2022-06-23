//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gnet.h"

namespace eda::gate::model {

GNet::GateId GNet::addGate(Gate *gate, GateFlags flags) {
  // ASSERT: the gate is not in this net.

  _gates.push_back(gate);
  _flags.insert({gate->id(), flags});

  return gate->id();
}

void GNet::removeGate(GateId id) {
  // ASSERT: the gate is in this net.

  // TODO:
}

void GNet::addNet(const GNet *net) {
  // ASSERT: this net and the given one are disjoint.
  assert(net);

  // TODO:
}

GNet::SubnetId GNet::addSubnet(GNet *subnet) {
  // ASSERT: this net and the subnet are disjoint.
  assert(subnet);

  SubnetId id = _subnets.size();
  _subnets.push_back(subnet);

  if (subnet->isEmpty()) {
    _emptySubnets.insert(id);
  } else {
    GateFlags flags{0, 0, id};
    for (auto *gate : subnet->gates()) {
      addGate(gate, flags);
    }
  }

  return id;
}

void GNet::moveGate(GateId id, SubnetId to) {
  // ASSERT: the gate is in the net.
  // ASSERT: the subnet is in the net.
  // TODO:
}

GNet::SubnetId GNet::mergeSubnets(SubnetId lhs, SubnetId rhs) {
  // ASSERT: the subnets are in the net.
  assert(lhs != rhs);

  // TODO:
  return lhs;
}

void GNet::flatten() {
  // TODO:
  _subnets.clear();
}

void GNet::removeEmptySubnets() {
  // TODO:
}

std::ostream& operator <<(std::ostream &out, const GNet &net) {
  for (const auto *gate: net.gates()) {
    out << *gate << std::endl;
  }
  return out;
}
 
} // namespace eda::gate::model
