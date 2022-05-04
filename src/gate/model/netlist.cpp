//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/netlist.h"

#include <algorithm>

namespace eda::gate::model {

Gate::Id Netlist::addGate(Gate *gate, GateFlags flags) {
  _gates.push_back(gate);
  _flags.insert({ gate->id(), flags });

  return gate->id();
}

void Netlist::addSubnet(Netlist *subnet) {
  _subnets.push_back(subnet);

  GateFlags flags{ 0, 0, static_cast<unsigned>(_subnets.size()) };
  for (auto *gate : subnet->gates()) {
    addGate(gate, flags);
  }
}

std::ostream& operator <<(std::ostream &out, const Netlist &netlist) {
  for (const auto *gate: netlist.gates()) {
    out << *gate << std::endl;
  }
  return out;
}
 
} // namespace eda::gate::model
