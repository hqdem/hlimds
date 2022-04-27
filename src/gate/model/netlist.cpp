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

void Netlist::setGate(Gate::Id id, GateSymbol kind, const Signal::List &inputs) {
  auto *gate = Gate::get(id);

  if (gate->isSource()) {
    auto i = std::find(_sources.begin(), _sources.end(), id);
    _sources.erase(i);
  } else if (gate->isTrigger()) {
    auto i = std::find(_triggers.begin(), _triggers.end(), id);
    _triggers.erase(i);
  }

  gate->setKind(kind);
  gate->setInputs(inputs);

  if (gate->isSource()) {
    _sources.push_back(id);
  } else if (gate->isTrigger()) {
    _triggers.push_back(id);
  }
}

std::ostream& operator <<(std::ostream &out, const Netlist &netlist) {
  for (const auto *gate: netlist.gates()) {
    out << *gate << std::endl;
  }
  return out;
}
 
} // namespace eda::gate::model
