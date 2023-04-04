//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gnet.h"
#include "gate/optimizer/rwdatabase.h"

#include <cstdlib>
#include <unordered_set>
#include <vector>

extern unsigned short s_RwrPracticalClasses[];
extern unsigned short s_RwtAigSubgraphs[];

namespace eda::gate::optimizer {

using Gate = eda::gate::model::Gate;
using GNet = eda::gate::model::GNet;
using GateId = Gate::Id;
using GateIdMap = std::unordered_map<GateId, GateId>;
using InputId = RWDatabase::InputId;
using TruthTable = RWDatabase::TruthTable;
using GateBindings = RWDatabase::GateBindings;
using BoundGNet = RWDatabase::BoundGNet;

/// The code below is based on ABC.
static BoundGNet getAbcRwDatabase(
    std::vector<std::pair<GateId, TruthTable>> &gates) {
  const unsigned short *graph = s_RwtAigSubgraphs;

  auto net = std::make_shared<GNet>();

  gates.push_back({net->addZero(), 0x0000}); // c0
  gates.push_back({net->addIn(),   0xAAAA}); // x1
  gates.push_back({net->addIn(),   0xCCCC}); // x2
  gates.push_back({net->addIn(),   0xF0F0}); // x3
  gates.push_back({net->addIn(),   0xFF00}); // x4

  GateBindings bindings;
  bindings[0] = gates[1].first;
  bindings[1] = gates[2].first;
  bindings[2] = gates[3].first;
  bindings[3] = gates[4].first;

  // Reconstruct the forest.
  for (size_t i = 0;; i++) {
    unsigned entry0 = graph[2*i + 0];
    unsigned entry1 = graph[2*i + 1];

    if (entry0 == 0 && entry1 == 0) {
      break;
    }

    // Get XOR flag.
    bool isXor = (entry0 & 1);
    entry0 >>= 1;

    // Get the nodes.
    const auto &[gid0, table0] = gates[entry0 >> 1];
    const auto &[gid1, table1] = gates[entry1 >> 1];

    const auto not0 = (entry0 & 1);
    const auto not1 = (entry1 & 1);

    const auto i0 = not0 ? net->addNot(gid0) : gid0;
    const auto i1 = not1 ? net->addNot(gid1) : gid1;
    const auto f0 = not0 ? ~table0 : table0;
    const auto f1 = not1 ? ~table1 : table1;

    const auto gid = (isXor ? net->addXor(i0, i1) : net->addAnd(i0, i1));
    const auto table = (isXor ? (f0 ^ f1) : (f0 & f1)) & 0xFFFF;
    gates.push_back({gid, table});
  }

  return {net, bindings};
}

static Gate::SignalList getNewInputs(const Gate::SignalList &oldInputs,
                                     const GateIdMap &oldToNewGates) {
  Gate::SignalList newInputs(oldInputs.size());

  for (size_t i = 0; i < oldInputs.size(); i++) {
    auto oldInput = oldInputs[i];
    auto newInput = oldToNewGates.find(oldInput.node());
    assert(newInput != oldToNewGates.end());

    newInputs[i] = Gate::Signal(oldInput.event(), newInput->second);
  }

  return newInputs;
}

static BoundGNet getCircuit(GateId gid, const BoundGNet &boundNet) {
  std::vector<GateId> gates;
  gates.push_back(gid);

  for (size_t i = 0; i < gates.size(); i++) {
    const auto *gate = Gate::get(gates[i]);

    for (auto input : gate->inputs()) {
      const auto inputId = input.node();
      gates.push_back(inputId);
    }
  }

  auto circuit = std::make_shared<GNet>();

  GateIdMap oldToNewGates;
  for (auto i = gates.rbegin(); i != gates.rend(); i++) {
    if (oldToNewGates.find(*i) == oldToNewGates.end()) {
      const auto *gate = Gate::get(*i);
      const auto newInputs = getNewInputs(gate->inputs(), oldToNewGates);
      const auto newGateId = circuit->addGate(gate->func(), newInputs);
      oldToNewGates[*i] = newGateId;
    }
  }

  circuit->addOut(oldToNewGates[gid]);

  GateBindings bindings;
  for (const auto &[id, gid] : boundNet.bindings) {
    if (oldToNewGates.find(gid) != oldToNewGates.end()) {
       bindings[id] = oldToNewGates[gid];
    }
  }

  return {circuit, bindings};
}

void initializeAbcRwDatabase(RWDatabase &database) {
  const unsigned short *classes = s_RwrPracticalClasses;

  std::vector<std::pair<GateId, TruthTable>> gates;
  auto net = getAbcRwDatabase(gates);

  std::unordered_set<TruthTable> practical;
  practical.reserve(135);
  practical.emplace(0);
  for (size_t i = 1; classes[i]; i++) {
    practical.emplace(classes[i]);
  }

  for (const auto &[gid, table] : gates) {
    if (practical.find(table) != practical.end()) {
      auto circuit = getCircuit(gid, net);

      std::cout << "Table: " << std::hex << table << std::endl;
      std::cout << std::dec << *circuit.net << std::endl;

      database.set(table, {circuit});
    }
  }
}

} // namespace eda::gate::optimizer
