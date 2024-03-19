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
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace eda::gate::optimizer2 {

model::SubnetID AbcNpn4Synthesizer::synthesize(
    const TruthTable &tt, uint16_t maxArity) {
  static constexpr auto k = 4;

  // Functions of more than 4 variables are not supported.
  if (tt.num_vars() > k) {
    return model::OBJ_NULL_ID;
  }

  const TruthTable ttk = tt.num_vars() < k ? kitty::extend_to(tt, k) : tt;

  const auto res = kitty::exact_npn_canonization(ttk);
  const auto npn = std::get<0>(res);

  const auto table = static_cast<uint16_t>(*npn.begin());

  const auto iterator = database.find(table);
  if (iterator == database.end()) {
    return model::OBJ_NULL_ID;
  }

  std::vector<size_t> cells;
  cells.push_back(iterator->second.idx);

  for (size_t i = 0; i < cells.size(); ++i) {
    for (auto link: builder.getLinks(cells[i])) {
      cells.push_back(link.idx);
    }
  }

  const auto n = std::get<1>(res); // Negations.
  const auto p = std::get<2>(res); // Permutation.

  // FIXME: Not very fast.
  model::SubnetBuilder subnetBuilder;
  std::unordered_map<size_t, model::Subnet::Link> oldToNewLink;

  const auto inputs = subnetBuilder.addInputs(k);
  for (size_t i = 0; i < k; ++i) {
    oldToNewLink[i] = n & (1 << i) ? ~inputs[i] : inputs[i];
  }

  for (auto i = cells.rbegin(); i != cells.rend(); ++i) {
    if (oldToNewLink.find(*i) == oldToNewLink.end()) {
      const auto &cell = builder.getCell(*i);

      auto links = builder.getLinks(*i);
      for (size_t j = 0; j < links.size(); ++j) {
        const auto oldLink = links[j];
        const auto newLink = oldToNewLink[oldLink.idx];
        links[j] = oldLink.inv ? ~newLink : newLink;
      }

      oldToNewLink[*i] = subnetBuilder.addCell(cell.getSymbol(), links);
    }
  }

  const auto newLink = oldToNewLink[cells[0]];
  subnetBuilder.addOutput(n & (1 << k) ? ~newLink : newLink);

  return subnetBuilder.make();
}

extern unsigned short s_RwrPracticalClasses[];
extern unsigned short s_RwtAigSubgraphs[];

/// The number of NPN classes of 4-variable functions.
static constexpr auto Npn4ClassNum = 222;

/// The code below is based on ABC.
static std::vector<std::pair<model::Subnet::Link, uint16_t>> buildAbcSubnet(
    model::SubnetBuilder &builder) {
  const unsigned short *graph = s_RwtAigSubgraphs;

  std::vector<std::pair<model::Subnet::Link, uint16_t>> links;
  links.reserve(Npn4ClassNum);

  // Create 4 inputs and a zero.
  const auto inputs = builder.addInputs(4);
  const auto const0 = builder.addCell(model::ZERO);

  links.emplace_back(const0,    0x0000); // Constant 0
  links.emplace_back(inputs[0], 0xaaaa); // Variable x0
  links.emplace_back(inputs[1], 0xcccc); // Variable x1
  links.emplace_back(inputs[2], 0xf0f0); // Variable x2
  links.emplace_back(inputs[3], 0xff00); // Varaible x3

  // Reconstruct the ABC forest.
  for (size_t i = 0;; ++i) {
    unsigned entry0 = graph[(i << 1) | 0];
    unsigned entry1 = graph[(i << 1) | 1];

    if (!entry0 && !entry1) {
      break;
    }

    // Get the XOR flag.
    bool isXor = (entry0 & 1);
    entry0 >>= 1;

    // Get the nodes.
    const auto &[link0, table0] = links[entry0 >> 1];
    const auto &[link1, table1] = links[entry1 >> 1];

    const auto not0 = (entry0 & 1);
    const auto not1 = (entry1 & 1);

    const auto l0 = not0 ? ~link0  : link0;
    const auto l1 = not1 ? ~link1  : link1;
    const auto t0 = not0 ? ~table0 : table0;
    const auto t1 = not1 ? ~table1 : table1;

    const auto link = builder.addCell(isXor ? model::XOR : model::AND, l0, l1);
    const auto table = (isXor ? (t0 ^ t1) : (t0 & t1)) & 0xffff;

    links.emplace_back(link, table);
  }

  return links;
}

static std::unordered_map<uint16_t, model::Subnet::Link> buildAbcDatabase(
    model::SubnetBuilder &builder) {
  const unsigned short *classes = s_RwrPracticalClasses;

  const auto links = buildAbcSubnet(builder);

  std::unordered_set<uint16_t> practicalNpnClasses;
  practicalNpnClasses.reserve(Npn4ClassNum);
  practicalNpnClasses.emplace(0);

  for (size_t i = 1; classes[i]; ++i) {
    practicalNpnClasses.emplace(classes[i]);
  }

  std::unordered_map<uint16_t, model::Subnet::Link> processedNpnClasses;
  processedNpnClasses.reserve(practicalNpnClasses.size());

  for (const auto &[link, table]: links) {
    if (practicalNpnClasses.find(table) != practicalNpnClasses.end() &&
        processedNpnClasses.find(table) == processedNpnClasses.end()) {
      processedNpnClasses.emplace(table, link);
    }
  }

  return processedNpnClasses;
}

AbcNpn4Synthesizer::AbcNpn4Synthesizer():
    builder(), database(buildAbcDatabase(builder)) {}

} // namespace eda::gate::optimizer2
