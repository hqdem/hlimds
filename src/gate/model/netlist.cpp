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

void Netlist::set_gate(unsigned id, GateSymbol kind, const Signal::List &inputs) {
  Gate *g = gate(id);

  if (g->is_source()) {
    auto i = std::find(_sources.begin(), _sources.end(), id);
    _sources.erase(i);
  } else if (g->is_trigger()) {
    auto i = std::find(_triggers.begin(), _triggers.end(), id);
    _triggers.erase(i);
  }

  g->set_kind(kind);
  g->set_inputs(inputs);

  if (g->is_source()) {
    _sources.push_back(id);
  } else if (g->is_trigger()) {
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
