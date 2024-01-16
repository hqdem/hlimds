//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021-2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/subnet.h"
#include "gate/model2/utils/subnet_cnf_encoder.h"
#include "gate/model2/utils/subnet_truth_table.h"

#include "gtest/gtest.h"
#include "kitty/print.hpp"

#include <iostream>
#include <vector>

namespace eda::gate::model {

SubnetID makeSimpleSubnet(CellSymbol symbol, size_t arity, uint16_t k) {
  using Link = Subnet::Link;
  using LinkList = Subnet::LinkList;

  SubnetBuilder builder;
  LinkList links;

  for (size_t i = 0; i < arity; i++) {
    const auto idx = builder.addInput();
    links.emplace_back(idx);
  }

  const auto idx = builder.addCellTree(symbol, links, k);
  builder.addOutput(Link(idx));

  return builder.make();
}

TEST(SubnetTest, SimpleTest) {
  using Link = Subnet::Link;

  static constexpr size_t Depth  = 3u;
  static constexpr size_t InNum  = (1u << Depth);
  static constexpr size_t OutNum = 1u;

  SubnetBuilder subnetBuilder;

  size_t idx[InNum];
  for (size_t i = 0; i < InNum; ++i) {
    idx[i] = subnetBuilder.addInput();
  }

  for (size_t n = (InNum >> 1); n != 0; n >>= 1) {
    for (size_t i = 0; i < n; ++i) {
      const Link lhs(idx[(i << 1)]);
      const Link rhs(idx[(i << 1) | 1]);

      idx[i] = subnetBuilder.addCell(((i & 1) ? AND : OR), lhs, rhs);
    }
  }

  subnetBuilder.addOutput(Link(idx[0]));

  const auto &subnet = Subnet::get(subnetBuilder.make());
  EXPECT_EQ(subnet.getInNum(), InNum);
  EXPECT_EQ(subnet.getOutNum(), OutNum);
  EXPECT_EQ(subnet.size(), 1u << (Depth + 1));

  std::cout << subnet << std::endl;
  std::cout << kitty::to_hex(evaluate(subnet)) << std::endl;

  const auto length = subnet.getPathLength();
  std::cout << "Path lenth: min=" << length.first
            << ", max="  << length.second << std::endl;

  eda::gate::solver::Solver solver;
  SubnetEncoder::get().encode(subnet, solver);
  EXPECT_TRUE(solver.solve());
}

TEST(SubnetTest, CellTreeTest) {
  static constexpr size_t Arity = 10u;
  static constexpr size_t K     = 2u;

  // OR.
  for (size_t i = 2; i < Arity; ++i) {
    const auto &subnet = Subnet::get(makeSimpleSubnet(OR, i, i));
    const auto &treeSubnet = Subnet::get(makeSimpleSubnet(OR, i, K));

    EXPECT_EQ(evaluate(subnet), evaluate(treeSubnet));
  }

  // AND.
  for (size_t i = 2; i < Arity; ++i) {
    const auto &subnet = Subnet::get(makeSimpleSubnet(AND, i, i));
    const auto &treeSubnet = Subnet::get(makeSimpleSubnet(AND, i, K));

    EXPECT_EQ(evaluate(subnet), evaluate(treeSubnet));
  }

  // XOR.
  for (size_t i = 2; i < Arity; ++i) {
    const auto &subnet = Subnet::get(makeSimpleSubnet(XOR, i, i));
    const auto &treeSubnet = Subnet::get(makeSimpleSubnet(XOR, i, K));

    EXPECT_EQ(evaluate(subnet), evaluate(treeSubnet));
  }
}

} // namespace eda::gate::model
