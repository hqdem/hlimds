//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gnet.h"
#include "gate/model/utils.h"
#include "gate/optimizer/rwdatabase.h"
#include "util/math.h"

#include <algorithm>
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
      const auto newInputs = model::getNewInputs(gate->inputs(), oldToNewGates);
      const auto newGateId = circuit->addGate(gate->func(), newInputs);
      oldToNewGates[*i] = newGateId;
    }
  }

  circuit->addOut(oldToNewGates[gid]);

  GateBindings bindings;
  InputId inputId = 0;
  for (const auto &[id, gid] : boundNet.bindings) {
    if (oldToNewGates.find(gid) != oldToNewGates.end()) {
      bindings[/*id*/inputId++] = oldToNewGates[gid]; // FIXME: There are holes in ids.
    }
  }

  return {circuit, bindings};
}

static TruthTable convertTruthTable(
    TruthTable table, size_t perm[], unsigned neg) {
  const unsigned k = 4;
  const unsigned size = (1 << k);
  const TruthTable x0 = 0xAAAA;
  const TruthTable x1 = 0xCCCC;
  const TruthTable x2 = 0xF0F0;
  const TruthTable x3 = 0xFF00;

  TruthTable x[k];
  TruthTable func = (neg & (1 << k)) ? ~table : table;

  x[perm[0]] = ((neg & (1 << 0)) ? ~x0 : x0);
  x[perm[1]] = ((neg & (1 << 1)) ? ~x1 : x1);
  x[perm[2]] = ((neg & (1 << 2)) ? ~x2 : x2);
  x[perm[3]] = ((neg & (1 << 3)) ? ~x3 : x3);

  unsigned array[size];
  for (unsigned i = 0; i < size; i++) {
    array[i] = ((func >> i) & 1) << 0
             | ((x[0] >> i) & 1) << 1
             | ((x[1] >> i) & 1) << 2
             | ((x[2] >> i) & 1) << 3
             | ((x[3] >> i) & 1) << 4;
  }

  std::sort(array, array + size);

  TruthTable result = 0;
  for (unsigned i = 0; i < size; i++) {
    result |= (array[i] & 1) << i;
  }

  return result;
}

static BoundGNet clone(const BoundGNet &circuit) {
  BoundGNet newCircuit;

  GateIdMap oldToNewGates;
  newCircuit.net = std::shared_ptr<GNet>(circuit.net->clone(oldToNewGates));

  for (const auto &[inputId, oldGateId] : circuit.bindings) {
    const auto newGateId = oldToNewGates[oldGateId];
    assert(newGateId != 0);

    newCircuit.bindings[inputId] = newGateId;
    std::cout << "inputId " << inputId << "->" << newGateId << std::endl;
  }

  return newCircuit;
}

static void generateNpnClasses(
    TruthTable table, const BoundGNet &boundNet, RWDatabase &database) {
  static size_t perm[24][4] = {
    {0, 1, 2, 3},
    {1, 0, 2, 3},
    {2, 0, 1, 3},
    {0, 2, 1, 3},
    {1, 2, 0, 3},
    {2, 1, 0, 3},
    {2, 1, 3, 0},
    {1, 2, 3, 0},
    {3, 2, 1, 0},
    {2, 3, 1, 0},
    {1, 3, 2, 0},
    {3, 1, 2, 0},
    {3, 0, 2, 1},
    {0, 3, 2, 1},
    {2, 3, 0, 1},
    {3, 2, 0, 1},
    {0, 2, 3, 1},
    {2, 0, 3, 1},
    {1, 0, 3, 2},
    {0, 1, 3, 2},
    {3, 1, 0, 2},
    {1, 3, 0, 2},
    {0, 3, 1, 2},
    {3, 0, 1, 2}
  };

  const size_t k = boundNet.bindings.size();
  const size_t N = 1 << (k + 1);
  const size_t P = utils::factorial(k);

  for (unsigned n = 0; n < N; n++) {
    // Clone the net consistently w/ the bindings.
    auto circuit = clone(boundNet);

    // Negate the inputs.
    for (unsigned i = 0; i < k; i++) {
      if ((n >> i) & 1) {
        auto oldInputId = circuit.bindings[i];
        auto newInputId = circuit.net->addIn();
        circuit.net->setNot(oldInputId, newInputId);
        circuit.bindings[i] = newInputId;
      }
    }

    // Negate the output.
    if ((n >> k) & 1) {
      auto oldOutputId = circuit.net->targetLinks().begin()->source; // TODO:
      auto *gate = Gate::get(oldOutputId);
      circuit.net->setNot(oldOutputId, gate->input(0));
      circuit.net->addOut(oldOutputId);
    }

    for (unsigned p = 0; p < P; p++) {
      for (unsigned i = 0; i < k; i++) {
        circuit.bindings[i] = perm[p][i];
      }

      const auto newTable = convertTruthTable(table, perm[p], n);
      std::cout << "Table: " << std::hex << newTable << std::endl;
      std::cout << std::dec << *circuit.net << std::endl;

      database.set(newTable, {circuit});
    }
  }
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

  std::unordered_set<TruthTable> processed;
  processed.reserve(practical.size());

  for (const auto &[gid, table] : gates) {
    if (practical.find(table) != practical.end() &&
        processed.find(table) == processed.end()) {
      auto circuit = getCircuit(gid, net);

      generateNpnClasses(table, circuit, database);
      processed.emplace(table);
    }
  }
}

} // namespace eda::gate::optimizer
