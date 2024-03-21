//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer2/synthesis/abc_npn4.h"

#include "kitty/kitty.hpp"

#include <cassert>
#include <cstddef>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

extern unsigned short s_RwrPracticalClasses[];
extern unsigned short s_RwtAigSubgraphs[];

namespace eda::gate::optimizer2 {

struct Database final {
  using TruthTable = AbcNpn4Synthesizer::TruthTable;

  using N = uint32_t;
  using P = std::vector<uint8_t>;

  static constexpr auto k = 4;

  static bool isVarIdx(size_t idx) {
    return 0 < idx && idx <= k;
  }

  static size_t var2Idx(size_t var) {
    assert(isVarIdx(var + 1));
    return var + 1;
  }

  static size_t idx2Var(size_t idx) {
    assert(isVarIdx(idx));
    return idx - 1;
  }

  static bool isNegVar(size_t var, N n) {
    return n & (1 << var);
  }

  static bool isNegOut(N n) {
    return n & (1 << k);
  }

  static bool negateIdx(size_t idx0, size_t idx1, N n0, N n1) {
    const auto neg0 = isVarIdx(idx0) ? isNegVar(idx2Var(idx0), n0) : false;
    const auto neg1 = isVarIdx(idx1) ? isNegVar(idx2Var(idx1), n1) : false;
    return neg0 ^ neg1;
  }

  static size_t permuteIdx(size_t idx, const P &p0, const P &p1) {
    return isVarIdx(idx) ? var2Idx(p1[p0[idx2Var(idx)]]) : idx;
  }

  struct Node final {
    Node():
        table(k), symbol(model::ZERO) {}

    Node(const TruthTable &table,
         model::CellSymbol symbol):
        table(table), symbol(symbol) {}

    Node(const TruthTable &table,
         model::CellSymbol symbol,
         model::Subnet::Link link0,
         model::Subnet::Link link1):
        table(table), symbol(symbol), link{link0, link1} {}

    const TruthTable table;
    const model::CellSymbol symbol;
    const model::Subnet::Link link[2];
  };

  struct Entry final {
    Entry():
        index(0), n(0), p{} {}

    Entry(size_t index, N n, const P &p):
        index(index), n(n), p(p) {}

    const size_t index;
    const N n;
    const P p;
  };

  Database();

  /// Returns the subnetID for the given table.
  model::SubnetID find(const TruthTable &tt) const;

  /// Stores precomputed AIGs for practical NPN classes.
  std::vector<Node> aig;
  /// Maps NPN-canonical truth tables to the links in the builder.
  std::unordered_map<uint16_t, Entry> map;
};

Database::Database() {
  static constexpr auto npn4Num = 222;

  aig.emplace_back();
  aig.emplace_back(kitty::nth_var<TruthTable>(k, 0), model::IN);
  aig.emplace_back(kitty::nth_var<TruthTable>(k, 1), model::IN);
  aig.emplace_back(kitty::nth_var<TruthTable>(k, 2), model::IN);
  aig.emplace_back(kitty::nth_var<TruthTable>(k, 3), model::IN);

  // Reconstruct the ABC forest.
  for (size_t i = 0;; ++i) {
    unsigned entry0 = s_RwtAigSubgraphs[(i << 1) | 0];
    unsigned entry1 = s_RwtAigSubgraphs[(i << 1) | 1];

    if (!entry0 && !entry1) {
      break;
    }

    const bool isXor = (entry0 & 1);
    entry0 >>= 1;

    const model::CellSymbol symbol = isXor ? model::XOR : model::AND;

    const model::Subnet::Link link0{entry0 >> 1, (entry0 & 1) != 0};
    const model::Subnet::Link link1{entry1 >> 1, (entry1 & 1) != 0};

    assert(link0.idx < aig.size());
    assert(link1.idx < aig.size());

    const auto tab0 = aig[link0.idx].table;
    const auto tab1 = aig[link1.idx].table;

    const auto arg0 = link0.inv ? ~tab0 : tab0;
    const auto arg1 = link1.inv ? ~tab1 : tab1;

    const auto table = isXor ? (arg0 ^ arg1) : (arg0 & arg1);

    aig.emplace_back(table, symbol, link0, link1);
  }

  // Initialize the table-to-index mapping.
  bool isPracticalNpn[1 << (1 << k)] = {false};
  isPracticalNpn[0x0000] = true;

  for (size_t i = 1; s_RwrPracticalClasses[i]; ++i) {
    isPracticalNpn[s_RwrPracticalClasses[i]] = true;
  }

  map.reserve(npn4Num);
  for (size_t i = 0; i < aig.size(); ++i) {
    const auto npnCanon = kitty::exact_npn_canonization(aig[i].table);
    const auto npnTable = static_cast<uint16_t>(*std::get<0>(npnCanon).begin());

    const auto n0 = std::get<1>(npnCanon);
    const auto p0 = std::get<2>(npnCanon);

    if (isPracticalNpn[npnTable] && map.find(npnTable) == map.end()) {
      map.emplace(npnTable, Entry{i, n0, p0});
    }
  }
}

model::SubnetID Database::find(const TruthTable &tt) const {
  const TruthTable ttk = tt.num_vars() < k ? kitty::extend_to(tt, k) : tt;

  const auto npnCanon = kitty::exact_npn_canonization(ttk);
  const auto npnTable = static_cast<uint16_t>(*std::get<0>(npnCanon).begin());

  const auto iterator = map.find(npnTable);
  if (iterator == map.end()) {
    return model::OBJ_NULL_ID;
  }

  const auto n0 = iterator->second.n;
  const auto p0 = iterator->second.p;

  bool isUsed[] = {
      false, // Constant 0
      false, // Variable x0
      false, // Variable x1
      false, // Variable x2
      false  // Variable x3
  };

  std::vector<size_t> indices;
  indices.push_back(iterator->second.index);

  for (size_t i = 0; i < indices.size(); ++i) {
    const auto &node = aig[indices[i]];

    if (node.symbol == model::ZERO || node.symbol == model::IN) {
      assert(indices[i] <= k);
      isUsed[indices[i]] = true;
    } else {
      indices.push_back(node.link[0].idx);
      indices.push_back(node.link[1].idx);
    }
  }

  const auto n1 = std::get<1>(npnCanon);
  const auto p1 = std::get<2>(npnCanon);

  model::SubnetBuilder builder;
  std::unordered_map<size_t, model::Subnet::Link> links;

  // Add inputs.
  for (size_t i = 0; i < tt.num_vars(); i++) {
    assert(i == 0 || !isUsed[i + 1] || isUsed[i]);

    const auto input = builder.addInput();
    links[i + 1] = isNegVar(i, n1) ? ~input : input;
  }

  // Add zero (if required).
  if (isUsed[0]) {
    links[0] = builder.addCell(model::ZERO);
  }

  for (auto i = indices.rbegin(); i != indices.rend(); ++i) {
    if (*i <= k || links.find(*i) != links.end()) {
      continue;
    }

    const auto &node = aig[*i];

    const auto i0 = node.link[0].idx;
    const auto i1 = node.link[1].idx;

    const auto j0 = permuteIdx(i0, p0, p1);
    const auto j1 = permuteIdx(i1, p0, p1);

    const auto neg0 = node.link[0].inv ^ negateIdx(i0, j0, n0, n1);
    const auto neg1 = node.link[1].inv ^ negateIdx(i1, j1, n0, n1);

    links[*i] = builder.addCell(node.symbol,
        neg0 ? ~links[j0] : links[j0],
        neg1 ? ~links[j1] : links[j1]);
  }

  const auto link = links[permuteIdx(indices[0], p0, p1)];
  builder.addOutput(isNegOut(n1) ? ~link : link);

  return builder.make();
}

AbcNpn4Synthesizer::AbcNpn4Synthesizer():
  cache(1 << (1 << Database::k)) {}

model::SubnetID AbcNpn4Synthesizer::synthesize(
    const TruthTable &tt, uint16_t maxArity) {
  static Database database;

  if (tt.num_vars() > Database::k) {
    return model::OBJ_NULL_ID;
  }

  const auto index = static_cast<uint16_t>(*tt.begin());
  if (cache[index] != model::OBJ_NULL_ID) {
    return cache[index];
  }

  return (cache[index] = database.find(tt));
}


} // namespace eda::gate::optimizer2
