//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021-2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/subnet.h"
#include "gate/model2/utils/subnet_checking.h"
#include "gate/model2/utils/subnet_cnf_encoder.h"
#include "gate/model2/utils/subnet_truth_table.h"

#include "gtest/gtest.h"
#include "kitty/print.hpp"

#include <iostream>
#include <vector>

namespace eda::gate::model {

static SubnetID makeTreeSubnet(CellSymbol symbol, size_t arity, uint16_t k) {
  SubnetBuilder builder;
  Subnet::LinkList links;

  for (size_t i = 0; i < arity; i++) {
    const auto idx = builder.addInput();
    links.emplace_back(idx);
  }

  const auto idx = builder.addCellTree(symbol, links, k);
  builder.addOutput(Subnet::Link(idx));

  return builder.make();
}

inline void checkMakeTreeSubnet(CellSymbol symbol, size_t arity, uint16_t k) {
  const auto &cellSubnet = Subnet::get(makeTreeSubnet(symbol, arity, arity));
  const auto &treeSubnet = Subnet::get(makeTreeSubnet(symbol, arity, k));

  EXPECT_TRUE(utils::checkArity(treeSubnet, k));
  EXPECT_EQ(evaluateSingleOut(cellSubnet), evaluateSingleOut(treeSubnet));
}

inline void testMakeTreeSubnet(CellSymbol symbol, size_t maxArity, uint16_t k) {
  for (size_t i = 2; i <= maxArity; ++i) {
    checkMakeTreeSubnet(symbol, i, k);
  }
}

TEST(SubnetTest, AddCellTreeTest) {
  constexpr size_t MaxArity = 10u;
  constexpr size_t K = 2u;

  checkMakeTreeSubnet(OR,  MaxArity, K);
  checkMakeTreeSubnet(AND, MaxArity, K);
  checkMakeTreeSubnet(XOR, MaxArity, K);
}

TEST(SubnetTest, AddCellTest) {
  constexpr size_t Depth  = 3u;
  constexpr size_t InNum  = (1u << Depth);
  constexpr size_t OutNum = 1u;

  SubnetBuilder builder;
  Subnet::LinkList links = builder.addInputs(InNum);

  for (size_t n = (InNum >> 1); n != 0; n >>= 1) {
    for (size_t i = 0; i < n; ++i) {
      const auto lhs = links[(i << 1)];
      const auto rhs = links[(i << 1) | 1];

      links[i] = builder.addCell(((i & 1) ? AND : OR), lhs, rhs);
    }
  }

  builder.addOutput(links[0]);

  const auto &subnet = Subnet::get(builder.make());
  EXPECT_EQ(subnet.getInNum(), InNum);
  EXPECT_EQ(subnet.getOutNum(), OutNum);
  EXPECT_EQ(subnet.size(), 1u << (Depth + 1));

  std::cout << subnet << std::endl;
  std::cout << kitty::to_hex(evaluateSingleOut(subnet)) << std::endl;

  const auto length = subnet.getPathLength();
  std::cout << "Path lenth: min=" << length.first
            << ", max="  << length.second << std::endl;

  eda::gate::solver::Solver solver;
  SubnetEncoder::get().encode(subnet, solver);
  EXPECT_TRUE(solver.solve());
}

TEST(SubnetTest, AddSingleOutputSubnetTest) {
  constexpr size_t InNum = 4;
  constexpr size_t SubnetNum = 4;
  constexpr size_t TotalInNum = InNum * SubnetNum;

  const auto subnetID = makeTreeSubnet(AND, InNum, 2);
  const auto &subnet = Subnet::get(subnetID);

  SubnetBuilder builder;

  Subnet::LinkList inputs = builder.addInputs(TotalInNum);
  Subnet::LinkList outputs(SubnetNum);

  for (size_t i = 0; i < SubnetNum; ++i) {
    Subnet::LinkList links(InNum);
    for (size_t j = 0; j < InNum; ++j) {
      links[j] = inputs[i*InNum + j];
    }

    outputs[i] = builder.addSingleOutputSubnet(subnetID, links);
  }

  builder.addOutputs(outputs);

  const auto &result = Subnet::get(builder.make());
  EXPECT_EQ(result.size(), SubnetNum * subnet.size());
}

TEST(SubnetTest, SimpleStrashTest) {
  constexpr size_t InNum = 5;
  constexpr size_t OutNum = 10;

  SubnetBuilder builder;

  Subnet::LinkList inputs = builder.addInputs(InNum);

  for (size_t i = 0; i < OutNum; ++i) {
    Subnet::Link link = builder.addCell(AND, inputs);
    builder.addOutput(link);
  }

  const auto &result = Subnet::get(builder.make());
  EXPECT_EQ(result.size(), InNum + OutNum + 1);
}

TEST(SubnetTest, SimpleMergeTest) {
  SubnetBuilder builder;

  Subnet::LinkList inputs = builder.addInputs(2);

  Subnet::Link link1 = builder.addCell(AND, inputs[0], inputs[1]);
  Subnet::Link link2 = builder.addCell(OR, ~inputs[0], ~inputs[1]);
  Subnet::Link link3 = builder.addCell(BUF, ~link2);

  builder.addOutput(link1);
  builder.addOutput(link3);

  SubnetBuilder::MergeMap mergeMap;
  SubnetBuilder::EntrySet entrySet;

  entrySet.insert(link3.idx);
  mergeMap[link1.idx] = entrySet;

  builder.mergeCells(mergeMap);

  const auto &result = Subnet::get(builder.make());
  std::cout << result << std::endl;
}

#if 0
TEST(SubnetTest, SimpleReplaceConstTest) {
  SubnetBuilder builder;

  Subnet::LinkList inputs = builder.addInputs(2);

  Subnet::Link link1 = builder.addCell(AND, inputs[0], inputs[1]);
  Subnet::Link link2 = builder.addCell(OR, ~inputs[0], ~inputs[1]);
  Subnet::Link link3 = builder.addCell(OR, link1, link2);

  builder.addOutput(link3);

  builder.replaceWithZero(SubnetBuilder::EntrySet{link3.idx});

  const auto &result = Subnet::get(builder.make());
  std::cout << result << std::endl;
}
#endif

} // namespace eda::gate::model
