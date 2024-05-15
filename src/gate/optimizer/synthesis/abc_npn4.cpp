//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#ifdef NPN4_USAGE_STATS
  #include "gate/optimizer/npn.h"
#endif // NPN4_USAGE_STATS
#include "gate/optimizer/synthesis/abc_npn4.h"

#include "kitty/kitty.hpp"

#include <cassert>
#include <cstddef>
#ifdef NPN4_USAGE_STATS
  #include <iostream>
#endif // NPN4_USAGE_STATS
#include <unordered_map>
#include <unordered_set>
#include <vector>

extern unsigned short s_RwrPracticalClasses[];
extern unsigned short s_RwtAigSubgraphs[];

namespace eda::gate::optimizer {

//===----------------------------------------------------------------------===//
// Database
//===----------------------------------------------------------------------===//

struct Database final {
  using TruthTable = AbcNpn4Synthesizer::TruthTable;

  using I = size_t;
  using N = uint32_t;
  using P = std::vector<uint8_t>;

  static constexpr auto k = AbcNpn4Synthesizer::k;

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

  static bool isNegIdx(size_t idx, N n) {
    return isVarIdx(idx) ? isNegVar(idx2Var(idx), n) : false;
  }

  static bool isNegIdx(size_t idx0, size_t idx1, N n0, N n1) {
    return isNegIdx(idx0, n0) ^ isNegIdx(idx1, n1);
  }

  static P invert(const P &p) {
    P inverted(p.size());
    for (size_t i = 0; i < p.size(); ++i) {
      inverted[p[i]] = i;
    }
    return inverted;
  }

  static size_t permIdx(size_t idx, const P &p0, const P &p1) {
    auto pi{invert(p0)};
    return isVarIdx(idx) ? var2Idx(p1[pi[idx2Var(idx)]]) : idx;
  }

  struct Node final {
    Node():
        table(k), symbol(model::ZERO) {}

    Node(const TruthTable &table, model::CellSymbol symbol):
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
    Entry(): i(0), n(0), p{} {}

    Entry(I i, N n, const P &p): i(i), n(n), p(p) {}

    const I i;
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

    // Self-checking.
    assert(kitty::create_from_npn_config(npnCanon) == aig[i].table);

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
  indices.push_back(iterator->second.i);

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
    links[i + 1] = builder.addInput();
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

    const auto j0 = permIdx(i0, p0, p1);
    const auto j1 = permIdx(i1, p0, p1);

    const auto neg0 = node.link[0].inv ^ isNegIdx(i0, j0, n0, n1);
    const auto neg1 = node.link[1].inv ^ isNegIdx(i1, j1, n0, n1);

    links[*i] = builder.addCell(node.symbol,
        neg0 ? ~links[j0] : links[j0],
        neg1 ? ~links[j1] : links[j1]);
  }

  const auto link = links[permIdx(indices[0], p0, p1)];
  builder.addOutput(isNegOut(n0) ^ isNegOut(n1) ? ~link : link);

  return builder.make();
}

static Database database;

//===----------------------------------------------------------------------===//
// AbcNpn4Synthesizer
//===----------------------------------------------------------------------===//

#ifdef NPN4_USAGE_STATS
  static uint32_t count[1 << (1 << Database::k)]{0};
#endif // NPN4_USAGE_STATS

model::SubnetID AbcNpn4Synthesizer::synthesize(
    const TruthTable &tt, uint16_t maxArity) const {

  static std::vector<SubnetID> cache[k + 1] {
      std::vector<SubnetID>(1 << (1 << 0)),
      std::vector<SubnetID>(1 << (1 << 1)),
      std::vector<SubnetID>(1 << (1 << 2)),
      std::vector<SubnetID>(1 << (1 << 3)),
      std::vector<SubnetID>(1 << (1 << 4)),
  };

#ifdef NPN4_USAGE_STATS
  const TruthTable ttk = tt.num_vars() < k ? kitty::extend_to(tt, k) : tt;

  const auto npnCanon = kitty::exact_npn_canonization(ttk);
  const auto npnTable = static_cast<uint16_t>(*std::get<0>(npnCanon).begin());

  count[npnTable]++;
#endif // NPN4_USAGE_STATS

  const auto n = tt.num_vars();

  if (n > k) {
    return model::OBJ_NULL_ID;
  }

  const auto index = static_cast<uint16_t>(*tt.begin());
  if (cache[n][index] != model::OBJ_NULL_ID) {
    return cache[n][index];
  }

  return (cache[n][index] = database.find(tt));
}

#ifdef NPN4_USAGE_STATS
void AbcNpn4Synthesizer::printNpn4UsageStats() {
  for (size_t i = 0; i < npn4Num; ++i) {
    std::cout << std::setfill('0') << std::setw(4)
              << std::hex << npn4[i] << ": "
              << std::dec << count[npn4[i]] << std::endl;
  }
}
#endif // NPN4_USAGE_STATS

} // namespace eda::gate::optimizer
