/*
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

  EXPECT_EQ(evaluate(cellSubnet), evaluate(treeSubnet));
}

inline void testMakeTreeSubnet(CellSymbol symbol, size_t maxArity, uint16_t k) {
  for (size_t i = 2; i <= maxArity; ++i) {
    checkMakeTreeSubnet(symbol, i, k);
  }
}

TEST(SubnetTest, CellTreeTest) {
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
  std::cout << kitty::to_hex(evaluate(subnet)) << std::endl;

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

TEST(SubnetTest, BugTest) {
  using Link = Subnet::Link;
  using LinkList = Subnet::LinkList;

  SubnetBuilder ANDSubnetBuilder;
  size_t idx[2];
  for (size_t i = 0; i < 2; ++i) {
    idx[i] = ANDSubnetBuilder.addInput();
  }
  auto idxAND = ANDSubnetBuilder.addCell(AND, idx[0], idx[1]);
  ANDSubnetBuilder.addOutput(Link(idxAND));
  SubnetID ANDSubnet = ANDSubnetBuilder.make();
  std::cout << "Subnet that will be used in SubnetBuilder" << std::endl;
  std::cout << Subnet::get(ANDSubnet) << std::endl;

  SubnetBuilder mainSubnetBuilder;
  LinkList linkList;
  for (size_t i = 0; i < 2; ++i) {
    linkList.emplace_back(mainSubnetBuilder.addInput());
  }
  auto idxANDSubnetOut0 = mainSubnetBuilder.addSingleOutputSubnet(ANDSubnet, linkList);
  std::cout << "idx of idxANDSubnetOut0: " << idxANDSubnetOut0.idx << std::endl;

  linkList.clear();
  for (size_t i = 0; i < 2; ++i) {
    linkList.emplace_back(mainSubnetBuilder.addInput());
  }
  auto idxANDSubnetOut1 = mainSubnetBuilder.addSingleOutputSubnet(ANDSubnet, linkList);
  std::cout << "idx of idxANDSubnetOut1: " << idxANDSubnetOut1.idx << std::endl;

  linkList.clear();
  linkList.emplace_back(idxANDSubnetOut0);
  linkList.emplace_back(idxANDSubnetOut1);
  auto idxANDSubnetOut2 = mainSubnetBuilder.addSingleOutputSubnet(ANDSubnet, linkList);
  std::cout << "idx of idxANDSubnetOut2: " << idxANDSubnetOut2.idx << std::endl;

  mainSubnetBuilder.addOutput(Link(idxANDSubnetOut2));
  SubnetID mainSubnetId = mainSubnetBuilder.make();

  std::cout << Subnet::get(mainSubnetId) << std::endl;
}

} // namespace eda::gate::model
*/
