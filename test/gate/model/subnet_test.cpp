//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021-2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/subnet.h"
#include "gate/model/utils/subnet_checking.h"
#include "gate/model/utils/subnet_cnf_encoder.h"
#include "gate/model/utils/subnet_truth_table.h"

#include "gtest/gtest.h"
#include "kitty/print.hpp"

#include <iostream>
#include <vector>

namespace eda::gate::model {

using DinTruthTable = kitty::dynamic_truth_table;

static bool truthTablesEqual(
    const SubnetID subnetID,
    const SubnetID targetSubnetID) {
  DinTruthTable t1 = evaluateSingleOut(Subnet::get(targetSubnetID));
  DinTruthTable t2 = evaluateSingleOut(Subnet::get(subnetID));
  return t1 == t2;
}

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

inline void checkFanoutsCorrect(
    const SubnetBuilder &builder,
    const std::vector<SubnetBuilder::FanoutsContainer> &correctFanouts) {
  for (const auto &entry : builder) {
    EXPECT_EQ(builder.getFanouts(entry), correctFanouts[entry]);
  }
}

inline void checkSessionsCorrect(
    const SubnetBuilder &builder,
    const std::vector<size_t> &correctSessions) {
  for (const auto entry : builder) {
    EXPECT_EQ(builder.getEntrySession(entry), correctSessions[entry]);
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

TEST(SubnetTest, AddPIAfterConst) {
  SubnetBuilder builder;

  Subnet::Link inputLink1 = builder.addInput();
  Subnet::Link link1 = builder.addCell(ZERO);
  Subnet::Link inputLink2 = builder.addInput();
  Subnet::Link link2 = builder.addCell(ONE);
  Subnet::Link inputLink3 = builder.addInput();

  builder.addOutput(link1);
  builder.addOutput(link2);
  builder.addOutput(inputLink2);

  Subnet::Link link3 = builder.addCell(AND, inputLink1, inputLink3, link2);

  builder.addOutput(link3);

  const auto subnetID = builder.make();

  std::cout << Subnet::get(subnetID) << '\n';
}

TEST(SubnetTest, DelBufs) {
  SubnetBuilder builder;

  Subnet::LinkList inputs = builder.addInputs(3);
  Subnet::Link link1 = builder.addCell(BUF, ~inputs[0]);
  Subnet::Link link2 = builder.addCell(BUF, link1);
  Subnet::Link link3 = builder.addCell(AND, inputs[1], inputs[2]);
  Subnet::Link link4 = builder.addCell(BUF, ~link3);
  Subnet::Link link5 = builder.addCell(AND, link2, link4);
  Subnet::Link link6 = builder.addCell(BUF, link5);
  builder.addOutput(link6);

  auto copyBuilder(builder);

  const auto noBufsSubnetID = builder.make(true);
  const auto bufsSubnetID = copyBuilder.make();

  std::cout << Subnet::get(noBufsSubnetID) << '\n';

  EXPECT_TRUE(truthTablesEqual(noBufsSubnetID, bufsSubnetID));
}

TEST(SubnetTest, DelBufWithOut) {
  SubnetBuilder builder;

  Subnet::LinkList inputs = builder.addInputs(3);
  Subnet::Link link1 = builder.addCell(AND, inputs[0], inputs[1], inputs[2]);
  Subnet::Link link2 = builder.addCell(BUF, ~link1);
  builder.addOutput(link2);

  auto copyBuilder(builder);

  const auto noBufsSubnetID = builder.make(true);
  const auto bufsSubnetID = copyBuilder.make();

  std::cout << Subnet::get(noBufsSubnetID) << '\n';

  EXPECT_TRUE(truthTablesEqual(noBufsSubnetID, bufsSubnetID));
}

TEST(SubnetTest, DelConnectedBufs) {
  SubnetBuilder builder;

  Subnet::LinkList inputs = builder.addInputs(2);
  Subnet::Link link1 = builder.addCell(BUF, ~inputs[0]);
  Subnet::Link link2 = builder.addCell(BUF, ~link1);
  Subnet::Link link3 = builder.addCell(AND, ~link2, inputs[1]);
  builder.addOutput(link3);

  auto copyBuilder(builder);

  const auto noBufsSubnetID = builder.make(true);
  const auto bufsSubnetID = copyBuilder.make();

  std::cout << Subnet::get(noBufsSubnetID) << '\n';

  EXPECT_TRUE(truthTablesEqual(noBufsSubnetID, bufsSubnetID));
}

TEST(SubnetTest, DelBufsCheckRefcount) {
  SubnetBuilder builder;

  Subnet::Link input = builder.addInput();
  Subnet::Link link1 = builder.addCell(BUF, input);
  Subnet::Link link2 = builder.addCell(BUF, link1);
  Subnet::Link link3 = builder.addCell(BUF, link2);
  builder.addOutput(input);
  builder.addOutput(link1);
  builder.addOutput(link2);
  builder.addOutput(link3);

  std::cout << Subnet::get(builder.make(true)) << '\n';

  EXPECT_EQ(builder.getCell(0).refcount, 4);
}

TEST(SubnetTest, DepthsAfterMake) {
  SubnetBuilder builder;

  Subnet::LinkList inputs = builder.addInputs(3);
  Subnet::Link link1 = builder.addCell(AND, inputs[0], inputs[1]);
  Subnet::Link link2 = builder.addCell(BUF, link1);
  Subnet::Link link3 = builder.addCell(BUF, link2);
  Subnet::Link link4 = builder.addCell(BUF, inputs[2]);
  Subnet::Link link5 = builder.addCell(AND, link3, link4);
  builder.addOutput(link5);

  std::cout << Subnet::get(builder.make(true)) << '\n';

  std::vector<size_t> correctDepths{0, 0, 0, 1, 2, 3};
  for (auto it = builder.begin(); it != builder.end(); ++it) {
    const size_t j = *it;
    EXPECT_EQ(correctDepths[j], builder.getDepth(j));
  }
}

TEST(SubnetTest, WeightsAfterMake) {
  SubnetBuilder builder;

  Subnet::LinkList inputs = builder.addInputs(3);
  Subnet::Link link1 = builder.addCell(AND, inputs[0], inputs[1]);
  Subnet::Link link2 = builder.addCell(BUF, link1);
  Subnet::Link link3 = builder.addCell(BUF, link2);
  Subnet::Link link4 = builder.addCell(BUF, inputs[2]);
  Subnet::Link link5 = builder.addCell(AND, link3, link4);
  builder.addOutput(link5);

  std::vector<float> weights{0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.45, 0.5};
  for (auto it = builder.begin(); it != builder.end(); ++it) {
    const size_t j = *it;
    builder.setWeight(j, weights[j]);
  }

  std::cout << Subnet::get(builder.make(true)) << '\n';

  std::vector<float> correctWeights{0.1, 0.15, 0.2, 0.25, 0.45, 0.5};
  for (auto it = builder.begin(); it != builder.end(); ++it) {
    size_t j = *it;
    EXPECT_EQ(correctWeights[j], builder.getWeight(j));
  }
}

TEST(SubnetTest, Fanouts) {
  SubnetBuilder builder;

  builder.enableFanouts();
  Subnet::LinkList inputs = builder.addInputs(4);
  Subnet::Link link1 = builder.addCell(AND, inputs[0], inputs[1], inputs[1]);
  Subnet::Link link2 = builder.addCell(AND, inputs[2], inputs[3]);
  Subnet::Link link3 = builder.addCell(AND, link1, link2);
  builder.addOutput(link3);

  checkFanoutsCorrect(builder, {{4}, {4, 4}, {5}, {5}, {6}, {6}, {7}, {}});
}

TEST(SubnetTest, FanoutsEnabling) {
  SubnetBuilder builder;

  Subnet::LinkList inputs = builder.addInputs(4);
  Subnet::Link link1 = builder.addCell(AND, inputs[0], inputs[1], inputs[1]);
  Subnet::Link link2 = builder.addCell(AND, inputs[2], inputs[3]);
  Subnet::Link link3 = builder.addCell(AND, link1, link2);
  builder.addOutput(link3);

  builder.enableFanouts();

  checkFanoutsCorrect(builder, {{4}, {4, 4}, {5}, {5}, {6}, {6}, {7}, {}});
}

TEST(SubnetTest, FanoutsReplace) {
  SubnetBuilder builder;

  builder.enableFanouts();

  Subnet::LinkList inputs = builder.addInputs(4);
  Subnet::Link link1 = builder.addCell(AND, inputs[0], inputs[1]);
  Subnet::Link link2 = builder.addCell(AND, inputs[2], inputs[3]);
  Subnet::Link link3 = builder.addCell(AND, link1, link2);
  builder.addOutput(link1);
  builder.addOutput(link3);

  SubnetBuilder rhsBuilder;

  Subnet::LinkList rhsInputs = rhsBuilder.addInputs(4);
  Subnet::Link rhsLink1 = rhsBuilder.addCell(OR, inputs[0], inputs[1],
      inputs[2], inputs[3]);
  Subnet::Link rhsLink2 = rhsBuilder.addCell(BUF, rhsLink1);
  rhsBuilder.addOutput(rhsLink2);
  const auto rhsID = rhsBuilder.make();

  std::unordered_map<size_t, size_t> mapping;
  mapping[0] = 0;
  mapping[1] = 1;
  mapping[2] = 2;
  mapping[3] = 3;
  mapping[6] = 6;

  builder.replace(rhsID, mapping);

  checkFanoutsCorrect(
      builder, {{4, 9}, {4, 9}, {9}, {9}, {7}, {}, {8}, {}, {}, {6}}
  );
}

TEST(SubnetTest, FanoutsReplaceTwice) {
  SubnetBuilder builder;

  builder.enableFanouts();

  Subnet::LinkList inputs = builder.addInputs(4);
  Subnet::Link link1 = builder.addCell(AND, inputs[0], inputs[1]);
  Subnet::Link link2 = builder.addCell(AND, inputs[2], inputs[3]);
  Subnet::Link link3 = builder.addCell(AND, link1, link2);
  builder.addOutput(link1);
  builder.addOutput(link3);

  // RHS 1
  SubnetBuilder rhsBuilder;

  Subnet::LinkList rhsInputs = rhsBuilder.addInputs(4);
  Subnet::Link rhsLink1 = rhsBuilder.addCell(OR, inputs[0], inputs[1],
      inputs[2], inputs[3]);
  Subnet::Link rhsLink2 = rhsBuilder.addCell(BUF, rhsLink1);
  rhsBuilder.addOutput(rhsLink2);
  const auto rhsID = rhsBuilder.make();

  std::unordered_map<size_t, size_t> mapping;
  mapping[0] = 0;
  mapping[1] = 1;
  mapping[2] = 2;
  mapping[3] = 3;
  mapping[6] = 6;

  builder.replace(rhsID, mapping);

  // RHS 2
  SubnetBuilder rhs2Builder;

  Subnet::LinkList rhs2Inputs = rhs2Builder.addInputs(4);
  Subnet::Link rhs2Link1 = rhs2Builder.addCell(AND, rhs2Inputs[0], rhs2Inputs[1]);
  Subnet::Link rhs2Link2 = rhs2Builder.addCell(AND, rhs2Inputs[2], rhs2Inputs[3]);
  Subnet::Link rhs2Link3 = rhs2Builder.addCell(AND, rhs2Link1, rhs2Link2);
  rhs2Builder.addOutput(rhs2Link3);
  const auto rhs2ID = rhs2Builder.make();

  std::unordered_map<size_t, size_t> mapping2;
  mapping2[0] = 0;
  mapping2[1] = 1;
  mapping2[2] = 2;
  mapping2[3] = 3;
  mapping2[7] = 6;

  builder.replace(rhs2ID, mapping2);

  checkFanoutsCorrect(builder, {{4}, {4}, {5}, {5}, {7, 6}, {6}, {8}, {}, {}});
}

TEST(SubnetTest, FanoutsLinksEntry) {
  SubnetBuilder builder;

  builder.enableFanouts();
  Subnet::LinkList inputs = builder.addInputs(6);
  Subnet::LinkList links{inputs[0], inputs[1], inputs[2], inputs[3], inputs[4],
      inputs[5]};
  Subnet::Link link1 = builder.addCell(AND, links);
  builder.addOutput(link1);

  checkFanoutsCorrect(builder, {{6}, {6}, {6}, {6}, {6}, {6}, {8}, {}, {}});
}

TEST(SubnetTest, FanoutsMerge) {
  SubnetBuilder builder;
  builder.enableFanouts();

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

  checkFanoutsCorrect(builder, {{2}, {2}, {5, 6}, {}, {}, {}, {}});
}

TEST(SubnetTest, SessionSimple) {
  SubnetBuilder builder;

  Subnet::LinkList inputs = builder.addInputs(3);
  Subnet::Link link1 = builder.addCell(AND, inputs[0], inputs[1]);
  Subnet::Link link2 = builder.addCell(OR, inputs[1], inputs[2]);
  Subnet::Link link3 = builder.addCell(AND, link1, link2);
  builder.addOutput(link3);

  builder.startSession();
  EXPECT_EQ(builder.getSession(), 1);
  for (const auto entry : builder) {
    if (entry != 2) {
      builder.markEntry(entry);
    }
  }

  checkSessionsCorrect(builder, {1, 1, 0, 1, 1, 1, 1});
}

TEST(SubnetTest, SessionReplaceDiff) {
  SubnetBuilder builder;

  Subnet::LinkList inputs = builder.addInputs(3);
  Subnet::Link link1 = builder.addCell(AND, inputs[0], inputs[1]);
  Subnet::Link link2 = builder.addCell(OR, inputs[1], inputs[2]);
  Subnet::Link link3 = builder.addCell(AND, link1, link2);
  builder.addOutput(link3);

  builder.startSession();
  EXPECT_EQ(builder.getSession(), 1);
  builder.endSession();
  builder.startSession();
  EXPECT_EQ(builder.getSession(), 2);
  for (const auto entry : builder) {
    builder.markEntry(entry);
  }

  // RHS SubnetBuilder
  SubnetBuilder rhsBuilder;

  const Subnet::LinkList rhsInputs = rhsBuilder.addInputs(3);
  const Subnet::Link rhsLink1 = rhsBuilder.addCell(AND, rhsInputs[0],
      rhsInputs[1], rhsInputs[2]);
  rhsBuilder.addOutput(rhsLink1);
  const auto rhsID = rhsBuilder.make();

  std::unordered_map<size_t, size_t> mapping;
  mapping[0] = 0;
  mapping[1] = 1;
  mapping[2] = 2;
  mapping[4] = 5;

  builder.replace(rhsID, mapping);

  checkSessionsCorrect(builder, {2, 2, 2, 0, 0, 0, 2});
}

TEST(SubnetTest, SessionReplaceSame) {
  SubnetBuilder builder;

  Subnet::LinkList inputs = builder.addInputs(2);
  Subnet::Link link1 = builder.addCell(AND, inputs[0], inputs[1]);
  builder.addOutput(link1);

  builder.startSession();
  EXPECT_EQ(builder.getSession(), 1);
  for (const auto entry : builder) {
    builder.markEntry(entry);
  }

  // RHS SubnetBuilder
  SubnetBuilder rhsBuilder;

  const Subnet::LinkList rhsInputs = rhsBuilder.addInputs(2);
  const Subnet::Link rhsLink1 = rhsBuilder.addCell(AND, rhsInputs[0],
      rhsInputs[1]);
  rhsBuilder.addOutput(rhsLink1);
  const auto rhsID = rhsBuilder.make();

  std::unordered_map<size_t, size_t> mapping;
  mapping[0] = 0;
  mapping[1] = 1;
  mapping[3] = 2;

  builder.replace(rhsID, mapping);

  checkSessionsCorrect(builder, {1, 1, 1, 1});
}

TEST(SubnetTest, SessionFillEmptyEntry) {
  SubnetBuilder builder;

  Subnet::LinkList inputs = builder.addInputs(2);
  Subnet::Link link1 = builder.addCell(BUF, inputs[0]);
  Subnet::Link link2 = builder.addCell(AND, link1, inputs[1]);
  builder.addOutput(link2);

  builder.startSession();
  EXPECT_EQ(builder.getSession(), 1);
  for (const auto entry : builder) {
    builder.markEntry(entry);
  }

  // RHS SubnetBuilder
  SubnetBuilder rhsBuilder;

  const Subnet::LinkList rhsInputs = rhsBuilder.addInputs(2);
  const Subnet::Link rhsLink1 = rhsBuilder.addCell(OR, rhsInputs[0],
      rhsInputs[1]);
  rhsBuilder.addOutput(rhsLink1);
  const auto rhsID = rhsBuilder.make();

  std::unordered_map<size_t, size_t> mapping;
  mapping[0] = 0;
  mapping[1] = 1;
  mapping[3] = 3;

  builder.replace(rhsID, mapping);

  builder.addOutput(inputs[0]);

  checkSessionsCorrect(builder, {1, 1, 0, 0, 1});
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
