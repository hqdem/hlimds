//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021-2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/subnet.h"
#include "gate/model2/utils/subnet_truth_table.h"

#include "gtest/gtest.h"
#include "kitty/print.hpp"

#include <iostream>
#include <vector>

namespace eda::gate::model {

SubnetID makeSimpleSubnet(CellSymbol symbol, size_t arity, bool tree = false) {
  using Link = Subnet::Link;
  using LinkList = Subnet::LinkList;

  SubnetBuilder builder;
  LinkList links;

  for (size_t i = 0; i < arity; i++) {
    const auto idx = builder.addCell(IN, SubnetBuilder::INPUT);
    links.emplace_back(Link(idx));
  }

  size_t idx = 0;
  if (tree) {
    idx = builder.addCellTree(symbol, links);
  } else {
    idx = builder.addCell(symbol, links);
  }

  builder.addCell(OUT, Link(idx), SubnetBuilder::OUTPUT);

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
    idx[i] = subnetBuilder.addCell(IN, SubnetBuilder::INPUT);
  }

  for (size_t n = (InNum >> 1); n != 0; n >>= 1) {
    for (size_t i = 0; i < n; ++i) {
      const Link lhs(idx[(i << 1)]);
      const Link rhs(idx[(i << 1) | 1]);

      idx[i] = subnetBuilder.addCell(((i & 1) ? AND : OR), lhs, rhs);
    }
  }

  subnetBuilder.addCell(OUT, Link(idx[0]), SubnetBuilder::OUTPUT);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  EXPECT_EQ(subnet.getInNum(), InNum);
  EXPECT_EQ(subnet.getOutNum(), OutNum);
  EXPECT_EQ(subnet.size(), 1u << (Depth + 1));

  std::cout << subnet << std::endl;
  std::cout << kitty::to_hex(evaluate(subnet)) << std::endl;

  const auto length = subnet.getPathLength();
  std::cout << "Path lenth: min=" << length.first
            << ", max="  << length.second << std::endl;
}

TEST(SubnetTest, CellTreeTest) {
  // OR.
  for (size_t i = 2; i < 6; ++i) {
    const auto &subnet = Subnet::get(makeSimpleSubnet(OR, i));
    const auto &treeSubnet = Subnet::get(makeSimpleSubnet(OR, i, true));

    EXPECT_EQ(evaluate(subnet), evaluate(treeSubnet));
  }

  // AND.
  for (size_t i = 2; i < 6; ++i) {
    const auto &subnet = Subnet::get(makeSimpleSubnet(AND, i));
    const auto &treeSubnet = Subnet::get(makeSimpleSubnet(AND, i, true));

    EXPECT_EQ(evaluate(subnet), evaluate(treeSubnet));
  }

  // XOR.
  for (size_t i = 2; i < 6; ++i) {
    const auto &subnet = Subnet::get(makeSimpleSubnet(XOR, i));
    const auto &treeSubnet = Subnet::get(makeSimpleSubnet(XOR, i, true));

    EXPECT_EQ(evaluate(subnet), evaluate(treeSubnet));
  }

  // XNOR.
  for (size_t i = 2; i < 6; ++i) {
    const auto &subnet = Subnet::get(makeSimpleSubnet(XNOR, i));
    const auto &treeSubnet = Subnet::get(makeSimpleSubnet(XNOR, i, true));

    EXPECT_EQ(evaluate(subnet), evaluate(treeSubnet));
  }
}

} // namespace eda::gate::model
