//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#define CHECK_EQUIVALENCE

#include "gate/debugger/sat_checker.h"
#include "gate/model/subnet.h"
#include "gate/optimizer/balancer.h"
#include "util/assert.h"

#include <chrono>
#include <filesystem>

#include "gtest/gtest.h"

namespace eda::gate::optimizer {

using SubnetBuilder = eda::gate::model::SubnetBuilder;

using Subnet = model::Subnet;
using SubnetID = model::SubnetID;
using SubnetBuilder = model::SubnetBuilder;
using SatChecker = eda::gate::debugger::SatChecker;

static void checkBalancingEquivalence(SubnetID lhs, SubnetID rhs) {
  SatChecker &checker = SatChecker::get();
  const auto &subnet = Subnet::get(lhs);
  const auto &opt = Subnet::get(rhs);

  std::cout << subnet.getInNum() << " ins " << subnet.getOutNum() << " outs\n";

  std::unordered_map<uint32_t, uint32_t> map;
  for (uint32_t i = 0; i < subnet.getInNum(); ++i) {
    map[i] = i;
  }

  for (uint32_t j = subnet.getOutNum(); j > 0; --j) {
    map[subnet.size() - j] = opt.size() - j;
  }

  EXPECT_TRUE(checker.areEquivalent(lhs, rhs, map).equal());
}

uint32_t getSubnetDepth(const SubnetBuilder &builder) {
  uint32_t depth = 0;
  for (auto it = builder.begin(); it != builder.end(); ++it) {
    depth = std::max(depth, builder.getDepth(*it));
  }
  return depth;
}

void printBalancingInfo(SubnetBuilder &builder,
                        const uint32_t expectedDepthBefore,
                        const uint32_t expectedDepthAfter) {

  const auto depthBeforeBalance = getSubnetDepth(builder);
  EXPECT_EQ(expectedDepthBefore, depthBeforeBalance);

  std::cout << "Net depth before balancing: " << depthBeforeBalance << std::endl;

  const auto subnetBeforeID = builder.make();
  auto copyBuilder = std::make_shared<SubnetBuilder>(subnetBeforeID);

  Balancer balancer("TestBalancer");

  const auto startBalancingTime{std::chrono::steady_clock::now()};

  balancer.transform(copyBuilder);

  const auto endBalancingTime{std::chrono::steady_clock::now()};
  const std::chrono::duration<double> balancingTime{endBalancingTime -
                                                    startBalancingTime};

  const SubnetID subnetAfterID = copyBuilder->make();
#ifdef CHECK_EQUIVALENCE
  checkBalancingEquivalence(subnetBeforeID, subnetAfterID);
#endif

  const auto depthAfterBalance = getSubnetDepth(*copyBuilder);
  std::cout << "Net depth after balancing: " << depthAfterBalance << std::endl;
  std::cout << "Balancing time: " << balancingTime.count() << " s" << std::endl;

  EXPECT_EQ(expectedDepthAfter, depthAfterBalance);
}

TEST(Balancer, SeveralLinksToSwap) {
  SubnetBuilder builder;
  const auto &inputs = builder.addInputs(5);
  const auto &andLink0 = builder.addCell(model::AND, inputs[3], inputs[4]);
  const auto &andLink1 = builder.addCell(model::AND, inputs[2], andLink0);
  const auto &andLink2 = builder.addCell(model::AND, inputs[0], inputs[1],
                                         andLink1);
  builder.addOutput(andLink2);

  printBalancingInfo(builder, 4, 3);
}

TEST(Balancer, AND) {
  SubnetBuilder builder;
  const auto &inputs = builder.addInputs(5);
  const auto &andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  const auto &andLink1 = builder.addCell(model::AND, inputs[2], andLink0);
  const auto &andLink2 = builder.addCell(model::AND, inputs[3], andLink1);
  const auto &andLink3 = builder.addCell(model::AND, inputs[4], andLink2);
  builder.addOutput(andLink3);

  printBalancingInfo(builder, 5, 4);
}

TEST(Balancer, AND2) {
  SubnetBuilder builder;
  const auto &inputs = builder.addInputs(4);
  const auto &andLink0 = builder.addCell(model::AND, inputs[2], inputs[1]);
  const auto &andLink1 = builder.addCell(model::AND, andLink0, inputs[0]);
  const auto &andLink2 = builder.addCell(model::AND, inputs[3], andLink1);
  builder.addOutput(andLink2);

  printBalancingInfo(builder, 4, 3);
}

TEST(Balancer, BalanceANDTwice) {
  SubnetBuilder builder;
  const auto &inputs = builder.addInputs(6);
  const auto &andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  const auto &andLink1 = builder.addCell(model::AND, andLink0, inputs[2]);
  const auto &andLink2 = builder.addCell(model::AND, andLink1, inputs[3]);
  const auto &andLink3 = builder.addCell(model::AND, andLink2, inputs[4]);
  const auto &andLink4 = builder.addCell(model::AND, andLink3, inputs[5]);
  builder.addOutput(andLink4);

  printBalancingInfo(builder, 6, 4);
}

TEST(Balancer, BalanceANDThrice) {
  SubnetBuilder builder;
  const auto &inputs = builder.addInputs(9);
  const auto &andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  const auto &andLink1 = builder.addCell(model::AND, andLink0, inputs[2]);
  const auto &andLink2 = builder.addCell(model::AND, andLink1, inputs[3]);
  const auto &andLink3 = builder.addCell(model::AND, inputs[5], inputs[6]);
  const auto &andLink4 = builder.addCell(model::AND, andLink3, inputs[7]);
  const auto &andLink5 = builder.addCell(model::AND, andLink4, inputs[8]);
  const auto &bufLink0 = builder.addCell(model::BUF, ~andLink5);
  const auto &andLink6 = builder.addCell(model::AND, inputs[4], bufLink0);
  const auto &andLink7 = builder.addCell(model::AND, andLink2, andLink6);
  builder.addOutput(andLink7);

  printBalancingInfo(builder, 7, 5);
}

TEST(Balancer, UnbalancableANDOR) {
  SubnetBuilder builder;
  const auto &inputs = builder.addInputs(9);
  const auto &andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  const auto &andLink1 = builder.addCell(model::AND, andLink0, inputs[2]);
  const auto &orLink0 = builder.addCell(model::OR, andLink1, inputs[3]);
  const auto &andLink2 = builder.addCell(model::AND, inputs[5], inputs[6]);
  const auto &orLink1 = builder.addCell(model::OR, andLink2, inputs[7]);
  const auto &andLink4 = builder.addCell(model::AND, orLink1, inputs[8]);
  const auto &bufLink0 = builder.addCell(model::BUF, ~andLink4);
  const auto &andLink5 = builder.addCell(model::AND, inputs[4], bufLink0);
  const auto &orLink2 = builder.addCell(model::OR, orLink0, andLink5);
  builder.addOutput(orLink2);

  printBalancingInfo(builder, 7, 7);
}

TEST(Balancer, OneInOneOut) {
  SubnetBuilder builder;
  const auto &inputs = builder.addInputs(1);
  builder.addOutput(inputs[0]);

  printBalancingInfo(builder, 1, 1);
}

TEST(Balancer, OR) {
  SubnetBuilder builder;
  const auto &inputs = builder.addInputs(4);
  const auto &orLink0 = builder.addCell(model::OR, inputs[0], inputs[1]);
  const auto &orLink1 = builder.addCell(model::OR, inputs[2], orLink0);
  const auto &orLink2 = builder.addCell(model::OR, inputs[3], orLink1);
  builder.addOutput(orLink2);

  printBalancingInfo(builder, 4, 3);
}

TEST(Balancer, XOR) {
  SubnetBuilder builder;
  const auto &inputs = builder.addInputs(4);
  const auto &xorLink0 = builder.addCell(model::XOR, inputs[0], inputs[1]);
  const auto &xorLink1 = builder.addCell(model::XOR, inputs[2], xorLink0);
  const auto &xorLink2 = builder.addCell(model::XOR, inputs[3], xorLink1);
  builder.addOutput(xorLink2);

  printBalancingInfo(builder, 4, 3);
}

TEST(Balancer, SeveralOut) {
  SubnetBuilder builder;
  const auto &inputs = builder.addInputs(4);
  const auto &andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  const auto &andLink1 = builder.addCell(model::AND, inputs[2], andLink0);
  builder.addOutput(andLink1);
  const auto &andLink2 = builder.addCell(model::AND, inputs[3], andLink1);
  builder.addOutput(andLink2);

  printBalancingInfo(builder, 4, 4);
}

TEST(Balancer, Arity3) {
  SubnetBuilder builder;
  const auto &inputs = builder.addInputs(6);
  const auto &andLink0 = builder.addCell(model::AND, inputs[1], inputs[2]);
  const auto &andLink1 = builder.addCell(model::AND, andLink0, inputs[3],
                                         inputs[4]);
  const auto &andLink2 = builder.addCell(model::AND, inputs[0], andLink1,
                                         inputs[5]);
  builder.addOutput(andLink2);

  printBalancingInfo(builder, 4, 3);
}

TEST(Balancer, Arity3_2) {
  SubnetBuilder builder;
  const auto &inputs = builder.addInputs(7);
  const auto &orLink0 = builder.addCell(model::OR, inputs[0], inputs[1]);
  const auto &orLink1 = builder.addCell(model::OR, inputs[2], inputs[3]);
  const auto &andLink0 = builder.addCell(model::AND, orLink0, orLink1,
                                         inputs[4]);
  const auto &andLink1 = builder.addCell(model::AND, andLink0, inputs[5],
                                         inputs[6]);
  builder.addOutput(andLink1);

  printBalancingInfo(builder, 4, 3);
}

TEST(Balancer, MajLeft) {
  SubnetBuilder builder;
  const auto &inputs = builder.addInputs(5);
  const auto &andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  const auto &majLink0 = builder.addCell(model::MAJ, andLink0, inputs[2],
                                         inputs[3]);
  const auto &majLink1 = builder.addCell(model::MAJ, majLink0, inputs[2],
                                         inputs[4]);
  builder.addOutput(majLink1);

  printBalancingInfo(builder, 4, 3);
}

TEST(Balancer, MajRight) {
  SubnetBuilder builder;
  const auto &inputs = builder.addInputs(5);
  const auto &andLink0 = builder.addCell(model::AND, inputs[3], inputs[4]);
  const auto &majLink0 = builder.addCell(model::MAJ, inputs[1], inputs[2],
                                         andLink0);
  const auto &majLink1 = builder.addCell(model::MAJ, inputs[0], inputs[2],
                                         majLink0);
  builder.addOutput(majLink1);

  printBalancingInfo(builder, 4, 3);
}

TEST(Balancer, MajUnbalancable) {
  SubnetBuilder builder;
  const auto &inputs = builder.addInputs(6);
  const auto &andLink0 = builder.addCell(model::AND, inputs[4], inputs[5]);
  const auto &majLink0 = builder.addCell(model::MAJ, inputs[2], inputs[3],
                                         andLink0);
  const auto &majLink1 = builder.addCell(model::MAJ, inputs[0], inputs[1],
                                         majLink0);
  builder.addOutput(majLink1);

  printBalancingInfo(builder, 4, 4);
}

TEST(Balancer, Maj2Options) {
  SubnetBuilder builder;
  const auto &inputs = builder.addInputs(6);
  const auto &majLink0 = builder.addCell(model::MAJ, inputs[0], inputs[2],
                                         inputs[1]);
  const auto &notLink0 = builder.addCell(model::BUF, ~inputs[5]);
  const auto &andLink0 = builder.addCell(model::AND, inputs[4], notLink0);
  const auto &majLink1 = builder.addCell(model::MAJ, inputs[3], inputs[2],
                                         andLink0);
  const auto &majLink2 = builder.addCell(model::MAJ, majLink0, inputs[2],
                                         majLink1);
  builder.addOutput(majLink2);

  printBalancingInfo(builder, 5, 4);
}

TEST(Balancer, BalanceMajTwice) {
  SubnetBuilder builder;
  const auto &inputs = builder.addInputs(6);
  const auto &andLink0 = builder.addCell(model::AND, inputs[4], inputs[5]);
  const auto &majLink0 = builder.addCell(model::MAJ, inputs[2], inputs[3],
                                         andLink0);
  const auto &majLink1 = builder.addCell(model::MAJ, inputs[1], inputs[3],
                                         majLink0);
  const auto &majLink2 = builder.addCell(model::MAJ, inputs[0], inputs[3],
                                         majLink1);
  builder.addOutput(majLink2);

  printBalancingInfo(builder, 5, 4);
}

} // namespace eda::gate::optimizer2
