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

model::SubnetID AbcNpn4Synthesizer::synthesize(
    const TruthTable &tt, uint16_t maxArity) {
  if (tt.num_vars() > Database::k) {
    return model::OBJ_NULL_ID;
  }

  const auto index = static_cast<uint16_t>(*tt.begin());
  if (cache[index] != model::OBJ_NULL_ID) {
    return cache[index];
  }

  return (cache[index] = database.find(tt));
}

AbcNpn4Synthesizer::Database::Database() {
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

    const auto table0 = aig[link0.idx].table;
    const auto table1 = aig[link1.idx].table;

    const auto t0 = link0.inv ? ~table0 : table0;
    const auto t1 = link1.inv ? ~table1 : table1;
    const auto table = isXor ? (t0 ^ t1) : (t0 & t1);

    aig.emplace_back(table, symbol, link0, link1);
  }

  // Initialize the table-to-index mapping.
  std::unordered_set<uint16_t> npn;
  npn.reserve(npn4Num);
  npn.insert(0x0000);

  for (size_t i = 1; s_RwrPracticalClasses[i]; ++i) {
    npn.insert(s_RwrPracticalClasses[i]);
  }

  map.reserve(npn.size());

  for (size_t i = 0; i < aig.size(); ++i) {
    const auto canon = kitty::exact_npn_canonization(aig[i].table);
    const auto table = static_cast<uint16_t>(*std::get<0>(canon).begin());

    // FIXME: Store negation and permutation.
    std::cout << "TABLE " << std::hex << table << " -> " << std::dec << i << std::endl;

    if (npn.find(table) != npn.end() && map.find(table) == map.end()) {
      std::cout << "ADD " << std::hex << table << " -> " << std::dec << i << std::endl;
      map.emplace(table, i);
    }
  }
}

model::SubnetID AbcNpn4Synthesizer::Database::find(const TruthTable &tt) const {
  static constexpr auto k = 4;

  // For 3 and less variables, the truth table is extended.
  const TruthTable ttk = tt.num_vars() < k ? kitty::extend_to(tt, k) : tt;

  const auto res = kitty::exact_npn_canonization(ttk);
  const auto npn = std::get<0>(res);
  const auto neg = std::get<1>(res);
  const auto per = std::get<2>(res);

  const auto table = static_cast<uint16_t>(*npn.begin());
  std::cout << "NPN Table: " << std::hex << table << std::endl;

  const auto iterator = map.find(table);
  if (iterator == map.end()) {
    return model::OBJ_NULL_ID;
  }

  bool isUsed[] = {
      false, // Constant 0
      false, // Variable x0
      false, // Variable x1
      false, // Variable x2
      false  // Variable x3
  };

  std::vector<size_t> indices;
  indices.push_back(iterator->second);

  for (size_t i = 0; i < indices.size(); ++i) {
    const auto &node = aig[indices[i]];

    if (node.symbol == model::ZERO || node.symbol == model::IN) {
      isUsed[indices[i]] = true;
    } else {
      indices.push_back(node.link[0].idx);
      indices.push_back(node.link[1].idx);
    }
  }

  model::SubnetBuilder builder;
  std::unordered_map<size_t, model::Subnet::Link> links;

  for (size_t i = 0; i < k; ++i) {
    if (isUsed[i + 1]) {
      const auto input = builder.addInput();
      links[i + 1] = neg & (1 << i) ? ~input : input;
    }
  }

  if (isUsed[0]) {
    links[0] = builder.addCell(model::ZERO);
  }

  for (auto i = indices.rbegin(); i != indices.rend(); ++i) {
    if (*i <= k || links.find(*i) != links.end()) {
      continue;
    }

    const auto &node = aig[*i];

    const auto link0 = links[node.link[0].idx];
    const auto link1 = links[node.link[1].idx];

    links[*i] = builder.addCell(node.symbol,
        node.link[0].inv ? ~link0 : link0,
        node.link[1].inv ? ~link1 : link1);
  }

  const auto link = links[indices[0]];
  builder.addOutput(neg & (1 << k) ? ~link : link);

  return builder.make();
}



} // namespace eda::gate::optimizer2
