//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021-2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/subnetview.h"
#include "gate/optimizer/safe_passer.h"

#include "gtest/gtest.h"

#include <iostream>
#include <unordered_map>

namespace eda::gate::model {

using SafePasser = optimizer::SafePasser;
using ReverseSafePasser = optimizer::ReverseSafePasser;

template<typename IterT>
void printCellsTrav(SubnetBuilder &builder, IterT it, IterT contaiterEnd) {
  for (; it != contaiterEnd; ++it) {
    const auto &cell = builder.getEntry(*it).cell;
    std::cout << "Current entry ID: " << *it << "; input entries IDs: ";
    for (uint64_t i = 0; i < cell.arity; ++i) {
      std::cout << cell.link[i].idx << ' ';
    }
    std::cout << '\n';
  }
  std::cout << '\n';
}

void printBidirectCellsTrav(SubnetBuilder &builder) {
  std::cout << "Forward entries traversal:\n";
  printCellsTrav(builder, (SafePasser)builder.begin(),
                 (SafePasser)builder.end());
  std::cout << "Reverse entries traversal:\n";
  printCellsTrav(builder, (ReverseSafePasser)builder.rbegin(),
                 (ReverseSafePasser)builder.rend());
}

bool linksEqual(const Subnet::Link &targetLink, const Subnet::Link &srcLink) {
  return targetLink.out == srcLink.out &&
         targetLink.inv == srcLink.inv;
}

bool cellsEqual(const Subnet::Cell &targetCell, const Subnet::Cell &srcCell) {
  if (!(targetCell.arity == srcCell.arity &&
        targetCell.flipFlop == srcCell.flipFlop &&
        targetCell.flipFlopID == srcCell.flipFlopID &&
        targetCell.more == srcCell.more &&
        targetCell.refcount == srcCell.refcount &&
        targetCell.type == srcCell.type)) {
    return false;
  }
  for (uint64_t i = 0; i < targetCell.arity; ++i) {
    if (!linksEqual(targetCell.link[i], srcCell.link[i])) {
      return false;
    }
  }
  return true;
}

// Checks subnets equality. This method works only for subnets which cells have
// the same topological order.
void subnetsEqual(const SubnetID target, const SubnetID src) {
  const auto &targetEntries = Subnet::get(target).getEntries();
  const auto &srcEntries = Subnet::get(src).getEntries();
  EXPECT_EQ(targetEntries.size(), srcEntries.size());
  for (std::size_t i = 0; i < targetEntries.size(); ++i) {
    const auto &targetCell = targetEntries[i].cell;
    const auto &srcCell = srcEntries[i].cell;
    EXPECT_TRUE(cellsEqual(targetCell, srcCell));
  }
}

void addCellsToBuilder1(SubnetBuilder &builder) {
  const auto inputs = builder.addInputs(3);
  const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  const auto orLink0 = builder.addCell(model::OR, inputs[1], inputs[2]);
  const auto xorLink0 = builder.addCell(model::XOR, andLink0, orLink0);
  builder.addOutput(xorLink0);
}

TEST(ReplaceTest, SimpleTest) {
  SubnetBuilder builder;
  const auto inputs = builder.addInputs(2);
  const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  builder.addOutput(andLink0);

  SubnetBuilder rhsBuilder;
  const auto rhsInputs = rhsBuilder.addInputs(2);
  const auto rhsBufLink0 = rhsBuilder.addCell(model::BUF, rhsInputs[0]);
  const auto rhsAndLink0 = rhsBuilder.addCell(model::AND, rhsBufLink0,
                                              rhsInputs[1]);
  rhsBuilder.addOutput(rhsAndLink0);

  const auto rhsID = rhsBuilder.make();
  InOutMapping mapping({inputs[0].idx, inputs[1].idx},
                                      {andLink0.idx});

  const auto effect = builder.evaluateReplace(rhsID, mapping);
  EXPECT_EQ(effect.size, -1);
  EXPECT_EQ(effect.depth, -1);
  builder.replace(rhsID, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto inputs = builder.addInputs(2);
    const auto bufLink0 = builder.addCell(model::BUF, inputs[0]);
    const auto andLink0 = builder.addCell(model::AND, bufLink0, inputs[1]);
    builder.addOutput(andLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceTest, SmallerRhs) {
  SubnetBuilder builder;
  addCellsToBuilder1(builder);

  SubnetBuilder rhsBuilder;
  const auto rhsInputs = rhsBuilder.addInputs(3);
  const auto rhsAndLink0 = rhsBuilder.addCell(model::AND, rhsInputs[0],
                                              rhsInputs[1], rhsInputs[2]);
  rhsBuilder.addOutput(rhsAndLink0);

  const auto rhsID = rhsBuilder.make();
  InOutMapping mapping({0, 1, 2}, {5});

  const auto effect = builder.evaluateReplace(rhsID, mapping);
  EXPECT_EQ(effect.size, 2);
  EXPECT_EQ(effect.depth, 1);
  builder.replace(rhsID, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto inputs = builder.addInputs(3);
    const auto xorLink0 = builder.addCell(model::AND, inputs[0],
                                                      inputs[1],
                                                      inputs[2]);
    builder.addOutput(xorLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceTest, LargerRhs) {
  SubnetBuilder builder;
  addCellsToBuilder1(builder);

  SubnetBuilder rhsBuilder;
  const auto rhsInputs = rhsBuilder.addInputs(3);
  const auto rhsBufLink0 = rhsBuilder.addCell(model::BUF, rhsInputs[0]);
  const auto rhsBufLink1 = rhsBuilder.addCell(model::BUF, rhsInputs[1]);
  const auto rhsBufLink2 = rhsBuilder.addCell(model::BUF, rhsInputs[2]);
  const auto rhsBufLink3 = rhsBuilder.addCell(model::BUF, rhsBufLink0);
  const auto rhsAndLink0 = rhsBuilder.addCell(model::AND, rhsBufLink3,
                                              rhsBufLink1, rhsBufLink2);
  rhsBuilder.addOutput(rhsAndLink0);

  const auto rhsID = rhsBuilder.make();
  InOutMapping mapping({0, 1, 2}, {5});

  const auto effect = builder.evaluateReplace(rhsID, mapping);
  EXPECT_EQ(effect.size, -2);
  EXPECT_EQ(effect.depth, -1);
  builder.replace(rhsID, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto inputs = builder.addInputs(3);
    const auto bufLink0 = builder.addCell(model::BUF, inputs[0]);
    const auto bufLink1 = builder.addCell(model::BUF, inputs[1]);
    const auto bufLink2 = builder.addCell(model::BUF, inputs[2]);
    const auto bufLink3 = builder.addCell(model::BUF, bufLink0);
    const auto andLink0 = builder.addCell(model::AND, bufLink3, bufLink1,
                                          bufLink2);
    builder.addOutput(andLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceTest, NoInner) {
  SubnetBuilder builder;
  addCellsToBuilder1(builder);

  SubnetBuilder rhsBuilder;
  const auto rhsInputs = rhsBuilder.addInputs(2);
  const auto rhsXorLink0 = rhsBuilder.addCell(model::XOR, rhsInputs[0],
                                              rhsInputs[1]);
  rhsBuilder.addOutput(rhsXorLink0);

  const auto rhsID = rhsBuilder.make();
  InOutMapping mapping({3, 4}, {5});

  const auto effect = builder.evaluateReplace(rhsID, mapping);
  EXPECT_EQ(effect.size, 0);
  EXPECT_EQ(effect.depth, 0);
  builder.replace(rhsID, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    addCellsToBuilder1(builder);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceTest, ReplaceTwice) {
  SubnetBuilder builder;
  addCellsToBuilder1(builder);

  SubnetBuilder rhsBuilder;
  const auto rhsInputs = rhsBuilder.addInputs(3);
  const auto rhsBufLink0 = rhsBuilder.addCell(model::BUF, rhsInputs[0]);
  const auto rhsBufLink1 = rhsBuilder.addCell(model::BUF, rhsInputs[1]);
  const auto rhsBufLink2 = rhsBuilder.addCell(model::BUF, rhsInputs[2]);
  const auto rhsAndLink0 = rhsBuilder.addCell(model::AND, rhsBufLink0,
                                              rhsBufLink1, rhsBufLink2);
  rhsBuilder.addOutput(rhsAndLink0);

  const auto rhsID = rhsBuilder.make();
  InOutMapping mapping1({0, 1, 2}, {5});

  const auto effect1 = builder.evaluateReplace(rhsID, mapping1);
  EXPECT_EQ(effect1.size, -1);
  EXPECT_EQ(effect1.depth, 0);
  builder.replace(rhsID, mapping1);
  printBidirectCellsTrav(builder);

  SubnetBuilder rhs2Builder;
  const auto rhs2Inputs = rhs2Builder.addInputs(1);
  const auto rhs2BufLink0 = rhs2Builder.addCell(model::BUF, rhs2Inputs[0]);
  const auto rhs2BufLink1 = rhs2Builder.addCell(model::BUF, rhs2BufLink0);
  rhs2Builder.addOutput(rhs2BufLink1);

  const auto rhs2ID = rhs2Builder.make();
  InOutMapping mapping2(EntryIDList{0}, EntryIDList{7});

  const auto effect2 = builder.evaluateReplace(rhs2ID, mapping2);
  EXPECT_EQ(effect2.size, -1);
  EXPECT_EQ(effect2.depth, -1);
  builder.replace(rhs2ID, mapping2);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto inputs = builder.addInputs(3);
    const auto bufLink0 = builder.addCell(model::BUF, inputs[0]);
    const auto bufLink1 = builder.addCell(model::BUF, bufLink0);
    const auto bufLink2 = builder.addCell(model::BUF, inputs[1]);
    const auto bufLink3 = builder.addCell(model::BUF, inputs[2]);
    const auto andLink0 = builder.addCell(model::AND, bufLink1, bufLink2,
                                          bufLink3);
    builder.addOutput(andLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceTest, OneCell) {
  SubnetBuilder builder;
  addCellsToBuilder1(builder);

  SubnetBuilder rhsBuilder;
  const auto rhsInputs = rhsBuilder.addInputs(1);
  rhsBuilder.addOutput(rhsInputs[0]);

  const auto rhsID = rhsBuilder.make();
  InOutMapping mapping(EntryIDList{3}, EntryIDList{3});

  const auto effect = builder.evaluateReplace(rhsID, mapping);
  EXPECT_EQ(effect.size, 0);
  EXPECT_EQ(effect.depth, 0);
  builder.replace(rhsID, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    addCellsToBuilder1(builder);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceTest, ExternalRefs) {
  SubnetBuilder builder;
  const auto inputs = builder.addInputs(4);
  const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  const auto andLink1 = builder.addCell(model::AND, inputs[1], inputs[2]);
  const auto andLink2 = builder.addCell(model::AND, inputs[2], inputs[3]);
  const auto orLink0 = builder.addCell(model::OR, andLink0, andLink1);
  const auto orLink1 = builder.addCell(model::OR, andLink1, andLink2);
  const auto xorLink0 = builder.addCell(model::XOR, orLink0, orLink1);
  builder.addOutput(xorLink0);

  SubnetBuilder rhsBuilder;
  const auto rhsInputs = rhsBuilder.addInputs(3);
  const auto rhsOrLink0 = rhsBuilder.addCell(model::OR, rhsInputs[0],
                                             rhsInputs[1], rhsInputs[2]);
  rhsBuilder.addOutput(rhsOrLink0);

  const auto rhsID = rhsBuilder.make();
  InOutMapping mapping({inputs[1].idx, inputs[2].idx, inputs[3].idx},
                                      {orLink1.idx});

  const auto effect = builder.evaluateReplace(rhsID, mapping);
  EXPECT_EQ(effect.size, 1);
  EXPECT_EQ(effect.depth, 1);
  builder.replace(rhsID, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto inputs = builder.addInputs(4);
    const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
    const auto andLink1 = builder.addCell(model::AND, inputs[1], inputs[2]);
    const auto orLink0 = builder.addCell(model::OR, andLink0, andLink1);
    const auto orLink1 = builder.addCell(model::OR, inputs[1], inputs[2],
                                         inputs[3]);
    const auto xorLink0 = builder.addCell(model::XOR, orLink0, orLink1);
    builder.addOutput(xorLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceTest, LessRootInputs) {
  SubnetBuilder builder;
  const auto inputs = builder.addInputs(3);
  const auto bufLink0 = builder.addCell(model::BUF, inputs[0]);
  const auto bufLink1 = builder.addCell(model::BUF, inputs[1]);
  const auto bufLink2 = builder.addCell(model::BUF, inputs[2]);
  const auto xorLink0 = builder.addCell(model::XOR, bufLink0, bufLink1,
                                        bufLink2);
  builder.addOutput(xorLink0);

  SubnetBuilder rhsBuilder;
  const auto rhsInputs = rhsBuilder.addInputs(3);
  const auto rhsXorLink0 = rhsBuilder.addCell(model::XOR, rhsInputs[0],
                                              rhsInputs[1], rhsInputs[2]);
  rhsBuilder.addOutput(rhsXorLink0);

  const auto rhsID = rhsBuilder.make();
  InOutMapping mapping({inputs[0].idx, inputs[1].idx, inputs[2].idx},
                                      {xorLink0.idx});

  const auto effect = builder.evaluateReplace(rhsID, mapping);
  EXPECT_EQ(effect.size, 3);
  EXPECT_EQ(effect.depth, 1);
  builder.replace(rhsID, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto inputs = builder.addInputs(3);
    const auto xorLink0 = builder.addCell(model::XOR, inputs[0], inputs[1],
                                          inputs[2]);
    builder.addOutput(xorLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceTest, InvLink) {
  SubnetBuilder builder;
  const auto inputs = builder.addInputs(2);
  const auto xorLink0 = builder.addCell(model::XOR, inputs[0], inputs[1]);
  builder.addOutput(xorLink0);

  SubnetBuilder rhsBuilder;
  const auto rhsInputs = rhsBuilder.addInputs(2);
  const auto rhsXorLink0 = rhsBuilder.addCell(model::XOR, ~rhsInputs[0],
                                              rhsInputs[1]);
  rhsBuilder.addOutput(rhsXorLink0);

  const auto rhsID = rhsBuilder.make();
  InOutMapping mapping({inputs[0].idx, inputs[1].idx},
                                      {xorLink0.idx});

  const auto effect = builder.evaluateReplace(rhsID, mapping);
  EXPECT_EQ(effect.size, 0);
  EXPECT_EQ(effect.depth, 0);
  builder.replace(rhsID, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto inputs = builder.addInputs(2);
    const auto xorLink0 = builder.addCell(model::XOR,
                                          Subnet::Link(inputs[0].idx, 1),
                                          inputs[1]);
    builder.addOutput(xorLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceTest, AddCellAfterReplace) {
  SubnetBuilder builder;
  const auto inputs = builder.addInputs(2);
  const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);

  SubnetBuilder rhsBuilder;
  const auto rhsInputs = rhsBuilder.addInputs(2);
  const auto rhsBufLink0 = rhsBuilder.addCell(model::BUF, rhsInputs[0]);
  const auto rhsBufLink1 = rhsBuilder.addCell(model::BUF, rhsInputs[1]);
  const auto rhsAndLink0 = rhsBuilder.addCell(model::AND, rhsBufLink0,
                                              rhsBufLink1);
  rhsBuilder.addOutput(rhsAndLink0);

  const auto rhsID = rhsBuilder.make();
  InOutMapping mapping({inputs[0].idx, inputs[1].idx},
                                      {andLink0.idx});

  const auto effect = builder.evaluateReplace(rhsID, mapping);
  EXPECT_EQ(effect.size, -2);
  EXPECT_EQ(effect.depth, -1);
  builder.replace(rhsID, mapping);
  printBidirectCellsTrav(builder);
  const auto bufLink0 = builder.addCell(model::BUF, andLink0);
  builder.addOutput(bufLink0);

  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto inputs = builder.addInputs(2);
    const auto bufLink0 = builder.addCell(model::BUF, inputs[0]);
    const auto bufLink1 = builder.addCell(model::BUF, inputs[1]);
    const auto andLink0 = builder.addCell(model::AND, bufLink0, bufLink1);
    const auto bufLink2 = builder.addCell(model::BUF, andLink0);
    builder.addOutput(bufLink2);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceTest, SameCone) {
  SubnetBuilder builder;
  addCellsToBuilder1(builder);

  SubnetView cone(builder, 5);
  const auto coneSubnetID = cone.getSubnet().make();

  const auto effect = builder.evaluateReplace(coneSubnetID, cone.getInOutMapping());
  EXPECT_EQ(effect.size, 0);
  EXPECT_EQ(effect.depth, 0);
  builder.replace(coneSubnetID, cone.getInOutMapping());
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    addCellsToBuilder1(builder);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceTest, DeleteCell) {
  SubnetBuilder builder;
  const auto &inputLink0 = builder.addInput();
  const auto &bufLink0 = builder.addCell(model::BUF, inputLink0);
  builder.addOutput(bufLink0);

  SubnetBuilder rhsBuilder;
  const auto &inLink0 = rhsBuilder.addInput();
  rhsBuilder.addOutput(inLink0);

  const auto rhsID = rhsBuilder.make();
  InOutMapping mapping(EntryIDList{0}, EntryIDList{1});

  const auto effect = builder.evaluateReplace(rhsID, mapping);
  EXPECT_EQ(effect.size, 1);
  EXPECT_EQ(effect.depth, 1);
  builder.replace(rhsID, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto &inLink0 = builder.addInput();
    const auto &bufLink0 = builder.addCell(model::BUF, inLink0);
    builder.addOutput(bufLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceTest, DeleteSeveralCells) {
  SubnetBuilder builder;
  const auto &inputLink0 = builder.addInput();
  const auto &bufLink0 = builder.addCell(model::BUF, inputLink0);
  const auto &bufLink1 = builder.addCell(model::BUF, bufLink0);
  const auto &bufLink2 = builder.addCell(model::BUF, bufLink1);
  builder.addOutput(bufLink2);

  SubnetBuilder rhsBuilder;
  const auto &inLink0 = rhsBuilder.addInput();
  rhsBuilder.addOutput(inLink0);

  const auto rhsID = rhsBuilder.make();
  InOutMapping mapping(EntryIDList{0}, EntryIDList{3});

  const auto effect = builder.evaluateReplace(rhsID, mapping);
  EXPECT_EQ(effect.size, 3);
  EXPECT_EQ(effect.depth, 3);
  builder.replace(rhsID, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto &inLink0 = builder.addInput();
    const auto &bufLink0 = builder.addCell(model::BUF, inLink0);
    builder.addOutput(bufLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceTest, DeleteCellWithInvOut) {
  SubnetBuilder builder;
  const auto &inputLink0 = builder.addInput();
  const auto &bufLink0 = builder.addCell(model::BUF, inputLink0);
  builder.addOutput(bufLink0);

  SubnetBuilder rhsBuilder;
  const auto &inLink0 = rhsBuilder.addInput();
  rhsBuilder.addOutput(~inLink0);

  const auto rhsID = rhsBuilder.make();
  InOutMapping mapping(EntryIDList{0}, EntryIDList{1});

  const auto effect = builder.evaluateReplace(rhsID, mapping);
  EXPECT_EQ(effect.size, 1);
  EXPECT_EQ(effect.depth, 1);
  builder.replace(rhsID, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto &inLink0 = builder.addInput();
    const auto &bufLink0 = builder.addCell(model::BUF, ~inLink0);
    builder.addOutput(bufLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceTest, InvertFanouts) {
  SubnetBuilder builder;
  addCellsToBuilder1(builder);

  SubnetBuilder rhsBuilder;
  const auto &inLinks = rhsBuilder.addInputs(2);
  const auto &andLink0 = rhsBuilder.addCell(model::AND, inLinks[0], inLinks[1]);
  rhsBuilder.addOutput(~andLink0);

  const auto rhsID = rhsBuilder.make();
  InOutMapping mapping({0, 1}, {3});

  const auto effect = builder.evaluateReplace(rhsID, mapping);
  EXPECT_EQ(effect.size, 0);
  EXPECT_EQ(effect.depth, 0);
  builder.replace(rhsID, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto inputs = builder.addInputs(3);
    const auto orLink0 = builder.addCell(model::OR, inputs[1], inputs[2]);
    const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
    const auto bufLink0 = builder.addCell(model::BUF, ~andLink0);
    const auto xorLink0 = builder.addCell(model::XOR, bufLink0, orLink0);
    builder.addOutput(xorLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceTest, DublicateRoot) {
  SubnetBuilder builder;
  const auto &inLinks = builder.addInputs(2);
  const auto &andLink0 = builder.addCell(model::AND, inLinks[0], inLinks[1]);
  const auto &xorLink0 = builder.addCell(model::XOR, andLink0, inLinks[1]);
  const auto &bufLink0 = builder.addCell(model::BUF, xorLink0);
  builder.addOutput(andLink0);
  builder.addOutput(bufLink0);

  SubnetBuilder rhsBuilder;
  const auto &rhsInLinks = rhsBuilder.addInputs(2);
  const auto &rhsAndLink0 = rhsBuilder.addCell(model::AND, rhsInLinks[0],
                                               rhsInLinks[1]);
  rhsBuilder.addOutput(rhsAndLink0);

  const auto rhsID = rhsBuilder.make();
  InOutMapping mapping({0, 1}, {4});

  const auto effect = builder.evaluateReplace(rhsID, mapping);
  EXPECT_EQ(effect.size, 2);
  EXPECT_EQ(effect.depth, 2);
  builder.replace(rhsID, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    // Unable to create subnet with dublicated cells.
    subnetsEqual(resultID, resultID);
  }
}

TEST(ReplaceTest, DeleteDublicatedRoot) {
  SubnetBuilder builder;
  const auto &inLinks = builder.addInputs(2);
  const auto &andLink0 = builder.addCell(model::AND, inLinks[0], inLinks[1]);
  const auto &xorLink0 = builder.addCell(model::XOR, andLink0, inLinks[1]);
  const auto &bufLink0 = builder.addCell(model::BUF, xorLink0);
  const auto &bufLink1 = builder.addCell(model::BUF, bufLink0);
  builder.addOutput(andLink0);
  builder.addOutput(bufLink1);

  SubnetBuilder rhsBuilder;
  const auto &rhsInLinks = rhsBuilder.addInputs(2);
  const auto &rhsAndLink0 = rhsBuilder.addCell(model::AND, rhsInLinks[0],
                                               rhsInLinks[1]);
  rhsBuilder.addOutput(rhsAndLink0);

  const auto rhsID = rhsBuilder.make();
  InOutMapping mapping1({0, 1}, {4});

  const auto effect1 = builder.evaluateReplace(rhsID, mapping1);
  EXPECT_EQ(effect1.size, 2);
  EXPECT_EQ(effect1.depth, 2);
  builder.replace(rhsID, mapping1);
  printBidirectCellsTrav(builder);

  SubnetBuilder rhs2Builder;
  const auto &rhs2InLinks = rhs2Builder.addInputs(2);
  const auto &rhs2XorLink0 = rhs2Builder.addCell(model::XOR, rhs2InLinks[0],
                                                 rhs2InLinks[1]);
  rhs2Builder.addOutput(rhs2XorLink0);

  const auto rhs2ID = rhs2Builder.make();
  InOutMapping mapping2({0, 1}, {5});

  const auto effect2 = builder.evaluateReplace(rhs2ID, mapping2);
  EXPECT_EQ(effect2.size, 1);
  EXPECT_EQ(effect2.depth, 2);
  builder.replace(rhs2ID, mapping2);
  printBidirectCellsTrav(builder);

  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto &inLinks = builder.addInputs(2);
    const auto &andLink0 = builder.addCell(model::AND, inLinks[0], inLinks[1]);
    const auto &xorLink0 = builder.addCell(model::XOR, inLinks[0], inLinks[1]);
    builder.addOutput(andLink0);
    builder.addOutput(xorLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceTest, ReuseReplacedCell) {
  SubnetBuilder builder;
  addCellsToBuilder1(builder);

  SubnetBuilder rhsBuilder;
  const auto &rhsInputLinks = rhsBuilder.addInputs(2);
  const auto &rhsAndLink0 = rhsBuilder.addCell(model::AND, rhsInputLinks[0],
                                               rhsInputLinks[1]);
  rhsBuilder.addOutput(rhsAndLink0);

  const auto rhsID = rhsBuilder.make();
  InOutMapping mapping1({1, 2}, {4});

  const auto effect1 = builder.evaluateReplace(rhsID, mapping1);
  EXPECT_EQ(effect1.size, 0);
  EXPECT_EQ(effect1.depth, 0);
  builder.replace(rhsID, mapping1);
  printBidirectCellsTrav(builder);

  SubnetBuilder rhs2Builder;
  const auto &rhs2InputLinks = rhs2Builder.addInputs(3);
  const auto &rhs2AndLink0 = rhs2Builder.addCell(model::AND, rhs2InputLinks[1],
                                                 rhs2InputLinks[2]);
  const auto &rhs2AndLink1 = rhs2Builder.addCell(model::AND, rhs2InputLinks[0],
                                                 rhs2AndLink0);
  rhs2Builder.addOutput(rhs2AndLink1);

  const auto rhs2ID = rhs2Builder.make();
  std::cout << Subnet::get(rhs2ID) << std::endl;
  InOutMapping mapping2({0, 1, 2}, {5});

  const auto effect2 = builder.evaluateReplace(rhs2ID, mapping2);
  EXPECT_EQ(effect2.size, 1);
  EXPECT_EQ(effect2.depth, 0);
  builder.replace(rhs2ID, mapping2);
  printBidirectCellsTrav(builder);

  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto &inputLinks = builder.addInputs(3);
    const auto &andLink0 = builder.addCell(model::AND, inputLinks[1],
                                           inputLinks[2]);
    const auto &andLink1 = builder.addCell(model::AND, inputLinks[0], andLink0);
    builder.addOutput(andLink1);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceTest, ReuseCellsFollowingRoot) {
  SubnetBuilder builder;
  const auto &inLinks = builder.addInputs(3);
  const auto &andLink0 = builder.addCell(model::AND, inLinks[0], inLinks[1]);
  const auto &andLink1 = builder.addCell(model::AND, inLinks[1], inLinks[2]);
  const auto &andLink2 = builder.addCell(model::AND, andLink0, andLink1);
  builder.addOutput(andLink2);
  const auto &orLink0 = builder.addCell(model::OR, inLinks[0], inLinks[1]);
  const auto &orLink1 = builder.addCell(model::OR, orLink0, inLinks[2]);
  const auto &orLink2 = builder.addCell(model::OR, orLink1, inLinks[2]);
  builder.addOutput(orLink2);

  SubnetBuilder rhsBuilder;
  const auto &rhsInLinks = rhsBuilder.addInputs(3);
  const auto &rhsOrLink0 = rhsBuilder.addCell(model::OR, rhsInLinks[0],
                                              rhsInLinks[1]);
  const auto &rhsOrLink1 = rhsBuilder.addCell(model::OR, rhsOrLink0,
                                              rhsInLinks[2]);
  const auto &rhsOrLink2 = rhsBuilder.addCell(model::OR, rhsOrLink1,
                                              rhsInLinks[2]);
  const auto &rhsBufLink0 = rhsBuilder.addCell(model::BUF, rhsOrLink2);
  rhsBuilder.addOutput(rhsBufLink0);

  const auto rhsID = rhsBuilder.make();
  InOutMapping mapping({0, 1, 2}, {5});

  const auto effect = builder.evaluateReplace(rhsID, mapping);
  EXPECT_EQ(effect.size, 2);
  EXPECT_EQ(effect.depth, -2);
  builder.replace(rhsID, mapping);
  printBidirectCellsTrav(builder);

  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto &inputLinks = builder.addInputs(3);
    const auto &orLink0 = builder.addCell(model::OR, inputLinks[0],
                                          inputLinks[1]);
    const auto &orLink1 = builder.addCell(model::OR, orLink0,
                                          inputLinks[2]);
    const auto &orLink2 = builder.addCell(model::OR, orLink1,
                                          inputLinks[2]);
    builder.addOutput(orLink2);
    const auto &bufLink0 = builder.addCell(model::BUF, orLink2);
    builder.addOutput(bufLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceTest, EvaluationTest) {
  SubnetBuilder builder;
  const auto &inLinks = builder.addInputs(3);
  const auto &andLink0 = builder.addCell(model::AND, inLinks[0], inLinks[1]);
  const auto &xorLink0 = builder.addCell(model::XOR, inLinks[1], inLinks[2]);
  const auto &orLink0 = builder.addCell(model::OR, andLink0, xorLink0);
  const auto &bufLink0 = builder.addCell(model::BUF, orLink0);
  const auto &bufLink1 = builder.addCell(model::BUF, ~orLink0);
  builder.addOutput(bufLink0);
  builder.addOutput(bufLink1);

  SubnetBuilder rhsBuilder;
  const auto &rhsInLinks = rhsBuilder.addInputs(3);
  const auto &rhsAndLink0 = rhsBuilder.addCell(model::AND, rhsInLinks[0],
                                               rhsInLinks[1]);
  const auto &rhsXorLink0 = rhsBuilder.addCell(model::XOR, rhsInLinks[1],
                                               inLinks[2]);
  const auto &rhsOrLink0 = rhsBuilder.addCell(model::OR, rhsAndLink0,
                                              rhsXorLink0);
  const auto &rhsBufLink0 = rhsBuilder.addCell(model::BUF, rhsOrLink0);
  rhsBuilder.addOutput(rhsBufLink0);

  const auto rhsID = rhsBuilder.make();
  InOutMapping mapping({0, 1, 2}, {6});

  const auto effect = builder.evaluateReplace(rhsID, mapping);
  EXPECT_EQ(effect.size, 0);
  EXPECT_EQ(effect.depth, 0);
}

TEST(ReplaceTest, OneEntryTraversal) {
  SubnetBuilder builder;
  builder.addInput();
  printBidirectCellsTrav(builder);
}

TEST(ReplaceTest, DanglingPIs) {
  SubnetBuilder builder;
  const auto &inLinks = builder.addInputs(4);
  const auto &andLink0 = builder.addCell(model::AND, inLinks[0], inLinks[1]);
  const auto &andLink1 = builder.addCell(model::AND, inLinks[2], inLinks[3]);
  const auto &andLink2 = builder.addCell(model::AND, andLink0, andLink1);
  const auto &andLink3 = builder.addCell(model::AND, inLinks[0], andLink2);
  const auto &andLink4 = builder.addCell(model::AND, andLink3, andLink1);
  builder.addOutput(andLink4);

  SubnetBuilder rhsBuilder;
  const auto &rhsInLinks = rhsBuilder.addInputs(2);
  const auto &rhsBufLink0 = rhsBuilder.addCell(model::BUF, rhsInLinks[1]);
  rhsBuilder.addOutput(rhsBufLink0);

  const auto rhsID = rhsBuilder.make();
  InOutMapping mapping({7, 5}, {8});

  const auto effect = builder.evaluateReplace(rhsID, mapping);
  EXPECT_EQ(effect.size, 3);
  EXPECT_EQ(effect.depth, 2);
}

TEST(ReplaceTest, ReusedAndNewLink) {
  SubnetBuilder builder;
  const auto &inLinks = builder.addInputs(3);
  const auto &andLink0 = builder.addCell(model::AND, inLinks[1], inLinks[2]);
  const auto &bufLink0 = builder.addCell(model::BUF, inLinks[0]);
  const auto &andLink1 = builder.addCell(model::AND, bufLink0, andLink0);
  builder.addOutput(andLink1);

  SubnetBuilder rhsBuilder;
  const auto &rhsInLinks = rhsBuilder.addInputs(3);
  const auto &rhsAndLink0 = rhsBuilder.addCell(model::AND, rhsInLinks[0],
      rhsInLinks[1]);
  const auto &rhsAndLink1 = rhsBuilder.addCell(model::AND, rhsInLinks[1],
      rhsInLinks[2]);
  const auto &rhsAndLink2 = rhsBuilder.addCell(model::AND, rhsAndLink0,
      rhsAndLink1);
  rhsBuilder.addOutput(rhsAndLink2);

  const auto rhsID = rhsBuilder.make();
  InOutMapping mapping({0, 1, 2}, {5});

  const auto effect = builder.evaluateReplace(rhsID, mapping);
  EXPECT_EQ(effect.size, 0);
  EXPECT_EQ(effect.depth, 0);
}

TEST(ReplaceBuilderTest, SimpleTest) {
  SubnetBuilder builder;
  const auto inputs = builder.addInputs(2);
  const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  builder.addOutput(andLink0);

  SubnetBuilder rhsBuilder;
  const auto rhsInputs = rhsBuilder.addInputs(2);
  const auto rhsBufLink0 = rhsBuilder.addCell(model::BUF, rhsInputs[0]);
  const auto rhsAndLink0 = rhsBuilder.addCell(model::AND, rhsBufLink0,
                                              rhsInputs[1]);
  rhsBuilder.addOutput(rhsAndLink0);

  InOutMapping mapping({inputs[0].idx, inputs[1].idx},
                                      {andLink0.idx});

  const auto effect = builder.evaluateReplace(rhsBuilder, mapping);
  EXPECT_EQ(effect.size, -1);
  EXPECT_EQ(effect.depth, -1);
  builder.replace(rhsBuilder, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto inputs = builder.addInputs(2);
    const auto bufLink0 = builder.addCell(model::BUF, inputs[0]);
    const auto andLink0 = builder.addCell(model::AND, bufLink0, inputs[1]);
    builder.addOutput(andLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceBuilderTest, SmallerRhs) {
  SubnetBuilder builder;
  addCellsToBuilder1(builder);

  SubnetBuilder rhsBuilder;
  const auto rhsInputs = rhsBuilder.addInputs(3);
  const auto rhsAndLink0 = rhsBuilder.addCell(model::AND, rhsInputs[0],
                                              rhsInputs[1], rhsInputs[2]);
  rhsBuilder.addOutput(rhsAndLink0);

  InOutMapping mapping({0, 1, 2}, {5});

  const auto effect = builder.evaluateReplace(rhsBuilder, mapping);
  EXPECT_EQ(effect.size, 2);
  EXPECT_EQ(effect.depth, 1);
  builder.replace(rhsBuilder, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto inputs = builder.addInputs(3);
    const auto xorLink0 = builder.addCell(model::AND, inputs[0],
                                                      inputs[1],
                                                      inputs[2]);
    builder.addOutput(xorLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceBuilderTest, LargerRhs) {
  SubnetBuilder builder;
  addCellsToBuilder1(builder);

  SubnetBuilder rhsBuilder;
  const auto rhsInputs = rhsBuilder.addInputs(3);
  const auto rhsBufLink0 = rhsBuilder.addCell(model::BUF, rhsInputs[0]);
  const auto rhsBufLink1 = rhsBuilder.addCell(model::BUF, rhsInputs[1]);
  const auto rhsBufLink2 = rhsBuilder.addCell(model::BUF, rhsInputs[2]);
  const auto rhsBufLink3 = rhsBuilder.addCell(model::BUF, rhsBufLink0);
  const auto rhsAndLink0 = rhsBuilder.addCell(model::AND, rhsBufLink3,
                                              rhsBufLink1, rhsBufLink2);
  rhsBuilder.addOutput(rhsAndLink0);

  InOutMapping mapping({0, 1, 2}, {5});

  const auto effect = builder.evaluateReplace(rhsBuilder, mapping);
  EXPECT_EQ(effect.size, -2);
  EXPECT_EQ(effect.depth, -1);
  builder.replace(rhsBuilder, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto inputs = builder.addInputs(3);
    const auto bufLink0 = builder.addCell(model::BUF, inputs[0]);
    const auto bufLink1 = builder.addCell(model::BUF, inputs[1]);
    const auto bufLink2 = builder.addCell(model::BUF, inputs[2]);
    const auto bufLink3 = builder.addCell(model::BUF, bufLink0);
    const auto andLink0 = builder.addCell(model::AND, bufLink3, bufLink1,
                                          bufLink2);
    builder.addOutput(andLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceBuilderTest, NoInner) {
  SubnetBuilder builder;
  addCellsToBuilder1(builder);

  SubnetBuilder rhsBuilder;
  const auto rhsInputs = rhsBuilder.addInputs(2);
  const auto rhsXorLink0 = rhsBuilder.addCell(model::XOR, rhsInputs[0],
                                              rhsInputs[1]);
  rhsBuilder.addOutput(rhsXorLink0);

  InOutMapping mapping({3, 4}, {5});

  const auto effect = builder.evaluateReplace(rhsBuilder, mapping);
  EXPECT_EQ(effect.size, 0);
  EXPECT_EQ(effect.depth, 0);
  builder.replace(rhsBuilder, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    addCellsToBuilder1(builder);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceBuilderTest, ReplaceTwice) {
  SubnetBuilder builder;
  addCellsToBuilder1(builder);

  SubnetBuilder rhsBuilder;
  const auto rhsInputs = rhsBuilder.addInputs(3);
  const auto rhsBufLink0 = rhsBuilder.addCell(model::BUF, rhsInputs[0]);
  const auto rhsBufLink1 = rhsBuilder.addCell(model::BUF, rhsInputs[1]);
  const auto rhsBufLink2 = rhsBuilder.addCell(model::BUF, rhsInputs[2]);
  const auto rhsAndLink0 = rhsBuilder.addCell(model::AND, rhsBufLink0,
                                              rhsBufLink1, rhsBufLink2);
  rhsBuilder.addOutput(rhsAndLink0);

  InOutMapping mapping1({0, 1, 2}, {5});

  const auto effect1 = builder.evaluateReplace(rhsBuilder, mapping1);
  EXPECT_EQ(effect1.size, -1);
  EXPECT_EQ(effect1.depth, 0);
  builder.replace(rhsBuilder, mapping1);
  printBidirectCellsTrav(builder);

  SubnetBuilder rhs2Builder;
  const auto rhs2Inputs = rhs2Builder.addInputs(1);
  const auto rhs2BufLink0 = rhs2Builder.addCell(model::BUF, rhs2Inputs[0]);
  const auto rhs2BufLink1 = rhs2Builder.addCell(model::BUF, rhs2BufLink0);
  rhs2Builder.addOutput(rhs2BufLink1);

  InOutMapping mapping2(EntryIDList{0}, EntryIDList{7});

  const auto effect2 = builder.evaluateReplace(rhs2Builder, mapping2);
  EXPECT_EQ(effect2.size, -1);
  EXPECT_EQ(effect2.depth, -1);
  builder.replace(rhs2Builder, mapping2);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto inputs = builder.addInputs(3);
    const auto bufLink0 = builder.addCell(model::BUF, inputs[0]);
    const auto bufLink1 = builder.addCell(model::BUF, bufLink0);
    const auto bufLink2 = builder.addCell(model::BUF, inputs[1]);
    const auto bufLink3 = builder.addCell(model::BUF, inputs[2]);
    const auto andLink0 = builder.addCell(model::AND, bufLink1, bufLink2,
                                          bufLink3);
    builder.addOutput(andLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceBuilderTest, OneCell) {
  SubnetBuilder builder;
  addCellsToBuilder1(builder);

  SubnetBuilder rhsBuilder;
  const auto rhsInputs = rhsBuilder.addInputs(1);
  rhsBuilder.addOutput(rhsInputs[0]);

  InOutMapping mapping(EntryIDList{3}, EntryIDList{3});

  const auto effect = builder.evaluateReplace(rhsBuilder, mapping);
  EXPECT_EQ(effect.size, 0);
  EXPECT_EQ(effect.depth, 0);
  builder.replace(rhsBuilder, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    addCellsToBuilder1(builder);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceBuilderTest, ExternalRefs) {
  SubnetBuilder builder;
  const auto inputs = builder.addInputs(4);
  const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  const auto andLink1 = builder.addCell(model::AND, inputs[1], inputs[2]);
  const auto andLink2 = builder.addCell(model::AND, inputs[2], inputs[3]);
  const auto orLink0 = builder.addCell(model::OR, andLink0, andLink1);
  const auto orLink1 = builder.addCell(model::OR, andLink1, andLink2);
  const auto xorLink0 = builder.addCell(model::XOR, orLink0, orLink1);
  builder.addOutput(xorLink0);

  SubnetBuilder rhsBuilder;
  const auto rhsInputs = rhsBuilder.addInputs(3);
  const auto rhsOrLink0 = rhsBuilder.addCell(model::OR, rhsInputs[0],
                                             rhsInputs[1], rhsInputs[2]);
  rhsBuilder.addOutput(rhsOrLink0);

  InOutMapping mapping({inputs[1].idx, inputs[2].idx, inputs[3].idx},
                                      {orLink1.idx});

  const auto effect = builder.evaluateReplace(rhsBuilder, mapping);
  EXPECT_EQ(effect.size, 1);
  EXPECT_EQ(effect.depth, 1);
  builder.replace(rhsBuilder, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto inputs = builder.addInputs(4);
    const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
    const auto andLink1 = builder.addCell(model::AND, inputs[1], inputs[2]);
    const auto orLink0 = builder.addCell(model::OR, andLink0, andLink1);
    const auto orLink1 = builder.addCell(model::OR, inputs[1], inputs[2],
                                         inputs[3]);
    const auto xorLink0 = builder.addCell(model::XOR, orLink0, orLink1);
    builder.addOutput(xorLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceBuilderTest, LessRootInputs) {
  SubnetBuilder builder;
  const auto inputs = builder.addInputs(3);
  const auto bufLink0 = builder.addCell(model::BUF, inputs[0]);
  const auto bufLink1 = builder.addCell(model::BUF, inputs[1]);
  const auto bufLink2 = builder.addCell(model::BUF, inputs[2]);
  const auto xorLink0 = builder.addCell(model::XOR, bufLink0, bufLink1,
                                        bufLink2);
  builder.addOutput(xorLink0);

  SubnetBuilder rhsBuilder;
  const auto rhsInputs = rhsBuilder.addInputs(3);
  const auto rhsXorLink0 = rhsBuilder.addCell(model::XOR, rhsInputs[0],
                                              rhsInputs[1], rhsInputs[2]);
  rhsBuilder.addOutput(rhsXorLink0);

  InOutMapping mapping({inputs[0].idx, inputs[1].idx, inputs[2].idx},
                                      {xorLink0.idx});

  const auto effect = builder.evaluateReplace(rhsBuilder, mapping);
  EXPECT_EQ(effect.size, 3);
  EXPECT_EQ(effect.depth, 1);
  builder.replace(rhsBuilder, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto inputs = builder.addInputs(3);
    const auto xorLink0 = builder.addCell(model::XOR, inputs[0], inputs[1],
                                          inputs[2]);
    builder.addOutput(xorLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceBuilderTest, InvLink) {
  SubnetBuilder builder;
  const auto inputs = builder.addInputs(2);
  const auto xorLink0 = builder.addCell(model::XOR, inputs[0], inputs[1]);
  builder.addOutput(xorLink0);

  SubnetBuilder rhsBuilder;
  const auto rhsInputs = rhsBuilder.addInputs(2);
  const auto rhsXorLink0 = rhsBuilder.addCell(model::XOR, ~rhsInputs[0],
                                              rhsInputs[1]);
  rhsBuilder.addOutput(rhsXorLink0);

  InOutMapping mapping({inputs[0].idx, inputs[1].idx},
                                      {xorLink0.idx});

  const auto effect = builder.evaluateReplace(rhsBuilder, mapping);
  EXPECT_EQ(effect.size, 0);
  EXPECT_EQ(effect.depth, 0);
  builder.replace(rhsBuilder, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto inputs = builder.addInputs(2);
    const auto xorLink0 = builder.addCell(model::XOR,
                                          Subnet::Link(inputs[0].idx, 1),
                                          inputs[1]);
    builder.addOutput(xorLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceBuilderTest, AddCellAfterReplace) {
  SubnetBuilder builder;
  const auto inputs = builder.addInputs(2);
  const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);

  SubnetBuilder rhsBuilder;
  const auto rhsInputs = rhsBuilder.addInputs(2);
  const auto rhsBufLink0 = rhsBuilder.addCell(model::BUF, rhsInputs[0]);
  const auto rhsBufLink1 = rhsBuilder.addCell(model::BUF, rhsInputs[1]);
  const auto rhsAndLink0 = rhsBuilder.addCell(model::AND, rhsBufLink0,
                                              rhsBufLink1);
  rhsBuilder.addOutput(rhsAndLink0);

  InOutMapping mapping({inputs[0].idx, inputs[1].idx},
                                      {andLink0.idx});

  const auto effect = builder.evaluateReplace(rhsBuilder, mapping);
  EXPECT_EQ(effect.size, -2);
  EXPECT_EQ(effect.depth, -1);
  builder.replace(rhsBuilder, mapping);
  printBidirectCellsTrav(builder);
  const auto bufLink0 = builder.addCell(model::BUF, andLink0);
  builder.addOutput(bufLink0);

  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto inputs = builder.addInputs(2);
    const auto bufLink0 = builder.addCell(model::BUF, inputs[0]);
    const auto bufLink1 = builder.addCell(model::BUF, inputs[1]);
    const auto andLink0 = builder.addCell(model::AND, bufLink0, bufLink1);
    const auto bufLink2 = builder.addCell(model::BUF, andLink0);
    builder.addOutput(bufLink2);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceBuilderTest, SameCone) {
  SubnetBuilder builder;
  addCellsToBuilder1(builder);

  SubnetBuilder rhsBuilder;
  addCellsToBuilder1(rhsBuilder);

  InOutMapping mapping({0, 1, 2}, {5});

  const auto effect = builder.evaluateReplace(rhsBuilder, mapping);
  EXPECT_EQ(effect.size, 0);
  EXPECT_EQ(effect.depth, 0);
  builder.replace(rhsBuilder, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    addCellsToBuilder1(builder);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceBuilderTest, DeleteCell) {
  SubnetBuilder builder;
  const auto &inputLink0 = builder.addInput();
  const auto &bufLink0 = builder.addCell(model::BUF, inputLink0);
  builder.addOutput(bufLink0);

  SubnetBuilder rhsBuilder;
  const auto &inLink0 = rhsBuilder.addInput();
  rhsBuilder.addOutput(inLink0);

  InOutMapping mapping(EntryIDList{0}, EntryIDList{1});

  const auto effect = builder.evaluateReplace(rhsBuilder, mapping);
  EXPECT_EQ(effect.size, 1);
  EXPECT_EQ(effect.depth, 1);
  builder.replace(rhsBuilder, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto &inLink0 = builder.addInput();
    const auto &bufLink0 = builder.addCell(model::BUF, inLink0);
    builder.addOutput(bufLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceBuilderTest, DeleteSeveralCells) {
  SubnetBuilder builder;
  const auto &inputLink0 = builder.addInput();
  const auto &bufLink0 = builder.addCell(model::BUF, inputLink0);
  const auto &bufLink1 = builder.addCell(model::BUF, bufLink0);
  const auto &bufLink2 = builder.addCell(model::BUF, bufLink1);
  builder.addOutput(bufLink2);

  SubnetBuilder rhsBuilder;
  const auto &inLink0 = rhsBuilder.addInput();
  rhsBuilder.addOutput(inLink0);

  InOutMapping mapping(EntryIDList{0}, EntryIDList{3});

  const auto effect = builder.evaluateReplace(rhsBuilder, mapping);
  EXPECT_EQ(effect.size, 3);
  EXPECT_EQ(effect.depth, 3);
  builder.replace(rhsBuilder, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto &inLink0 = builder.addInput();
    const auto &bufLink0 = builder.addCell(model::BUF, inLink0);
    builder.addOutput(bufLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceBuilderTest, DeleteCellWithInvOut) {
  SubnetBuilder builder;
  const auto &inputLink0 = builder.addInput();
  const auto &bufLink0 = builder.addCell(model::BUF, inputLink0);
  builder.addOutput(bufLink0);

  SubnetBuilder rhsBuilder;
  const auto &inLink0 = rhsBuilder.addInput();
  rhsBuilder.addOutput(~inLink0);

  InOutMapping mapping(EntryIDList{0}, EntryIDList{1});

  const auto effect = builder.evaluateReplace(rhsBuilder, mapping);
  EXPECT_EQ(effect.size, 1);
  EXPECT_EQ(effect.depth, 1);
  builder.replace(rhsBuilder, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto &inLink0 = builder.addInput();
    const auto &bufLink0 = builder.addCell(model::BUF, ~inLink0);
    builder.addOutput(bufLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceBuilderTest, DeleteCellSeveralIns) {
  SubnetBuilder builder;
  const auto &inputLinks = builder.addInputs(2);
  const auto &andLink0 = builder.addCell(model::AND, inputLinks[0],
      inputLinks[1]);
  builder.addOutput(andLink0);

  SubnetBuilder rhsBuilder;
  const auto &rhsInLinks = rhsBuilder.addInputs(2);
  rhsBuilder.addOutput(rhsInLinks[1]);

  InOutMapping mapping({0, 1}, {2});

  const auto effect = builder.evaluateReplace(rhsBuilder, mapping);
  EXPECT_EQ(effect.size, 1);
  EXPECT_EQ(effect.depth, 1);
  builder.replace(rhsBuilder, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto &inLinks = builder.addInputs(2);
    const auto &bufLink0 = builder.addCell(model::BUF, inLinks[1]);
    builder.addOutput(bufLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceBuilderTest, InvertFanouts) {
  SubnetBuilder builder;
  addCellsToBuilder1(builder);

  SubnetBuilder rhsBuilder;
  const auto &inLinks = rhsBuilder.addInputs(2);
  const auto &andLink0 = rhsBuilder.addCell(model::AND, inLinks[0], inLinks[1]);
  rhsBuilder.addOutput(~andLink0);

  InOutMapping mapping({0, 1}, {3});

  const auto effect = builder.evaluateReplace(rhsBuilder, mapping);
  EXPECT_EQ(effect.size, 0);
  EXPECT_EQ(effect.depth, 0);
  builder.replace(rhsBuilder, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto inputs = builder.addInputs(3);
    const auto orLink0 = builder.addCell(model::OR, inputs[1], inputs[2]);
    const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
    const auto bufLink0 = builder.addCell(model::BUF, ~andLink0);
    const auto xorLink0 = builder.addCell(model::XOR, bufLink0, orLink0);
    builder.addOutput(xorLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceBuilderTest, DublicateRoot) {
  SubnetBuilder builder;
  const auto &inLinks = builder.addInputs(2);
  const auto &andLink0 = builder.addCell(model::AND, inLinks[0], inLinks[1]);
  const auto &xorLink0 = builder.addCell(model::XOR, andLink0, inLinks[1]);
  const auto &bufLink0 = builder.addCell(model::BUF, xorLink0);
  builder.addOutput(andLink0);
  builder.addOutput(bufLink0);

  SubnetBuilder rhsBuilder;
  const auto &rhsInLinks = rhsBuilder.addInputs(2);
  const auto &rhsAndLink0 = rhsBuilder.addCell(model::AND, rhsInLinks[0],
                                               rhsInLinks[1]);
  rhsBuilder.addOutput(rhsAndLink0);

  InOutMapping mapping({0, 1}, {4});

  const auto effect = builder.evaluateReplace(rhsBuilder, mapping);
  EXPECT_EQ(effect.size, 2);
  EXPECT_EQ(effect.depth, 2);
  builder.replace(rhsBuilder, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    // Unable to create subnet with dublicated cells.
    subnetsEqual(resultID, resultID);
  }
}

TEST(ReplaceBuilderTest, DeleteDublicatedRoot) {
  SubnetBuilder builder;
  const auto &inLinks = builder.addInputs(2);
  const auto &andLink0 = builder.addCell(model::AND, inLinks[0], inLinks[1]);
  const auto &xorLink0 = builder.addCell(model::XOR, andLink0, inLinks[1]);
  const auto &bufLink0 = builder.addCell(model::BUF, xorLink0);
  const auto &bufLink1 = builder.addCell(model::BUF, bufLink0);
  builder.addOutput(andLink0);
  builder.addOutput(bufLink1);

  SubnetBuilder rhsBuilder;
  const auto &rhsInLinks = rhsBuilder.addInputs(2);
  const auto &rhsAndLink0 = rhsBuilder.addCell(model::AND, rhsInLinks[0],
                                               rhsInLinks[1]);
  rhsBuilder.addOutput(rhsAndLink0);

  InOutMapping mapping1({0, 1}, {4});

  const auto effect1 = builder.evaluateReplace(rhsBuilder, mapping1);
  EXPECT_EQ(effect1.size, 2);
  EXPECT_EQ(effect1.depth, 2);
  builder.replace(rhsBuilder, mapping1);
  printBidirectCellsTrav(builder);

  SubnetBuilder rhs2Builder;
  const auto &rhs2InLinks = rhs2Builder.addInputs(2);
  const auto &rhs2XorLink0 = rhs2Builder.addCell(model::XOR, rhs2InLinks[0],
                                                 rhs2InLinks[1]);
  rhs2Builder.addOutput(rhs2XorLink0);

  InOutMapping mapping2({0, 1}, {5});

  const auto effect2 = builder.evaluateReplace(rhs2Builder, mapping2);
  EXPECT_EQ(effect2.size, 1);
  EXPECT_EQ(effect2.depth, 2);
  builder.replace(rhs2Builder, mapping2);
  printBidirectCellsTrav(builder);

  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto &inLinks = builder.addInputs(2);
    const auto &andLink0 = builder.addCell(model::AND, inLinks[0], inLinks[1]);
    const auto &xorLink0 = builder.addCell(model::XOR, inLinks[0], inLinks[1]);
    builder.addOutput(andLink0);
    builder.addOutput(xorLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceBuilderTest, ReuseReplacedCell) {
  SubnetBuilder builder;
  addCellsToBuilder1(builder);

  SubnetBuilder rhsBuilder;
  const auto &rhsInputLinks = rhsBuilder.addInputs(2);
  const auto &rhsAndLink0 = rhsBuilder.addCell(model::AND, rhsInputLinks[0],
                                               rhsInputLinks[1]);
  rhsBuilder.addOutput(rhsAndLink0);

  InOutMapping mapping1({1, 2}, {4});

  const auto effect1 = builder.evaluateReplace(rhsBuilder, mapping1);
  EXPECT_EQ(effect1.size, 0);
  EXPECT_EQ(effect1.depth, 0);
  builder.replace(rhsBuilder, mapping1);
  printBidirectCellsTrav(builder);

  SubnetBuilder rhs2Builder;
  const auto &rhs2InputLinks = rhs2Builder.addInputs(3);
  const auto &rhs2AndLink0 = rhs2Builder.addCell(model::AND, rhs2InputLinks[1],
                                                 rhs2InputLinks[2]);
  const auto &rhs2AndLink1 = rhs2Builder.addCell(model::AND, rhs2InputLinks[0],
                                                 rhs2AndLink0);
  rhs2Builder.addOutput(rhs2AndLink1);

  InOutMapping mapping2({0, 1, 2}, {5});

  const auto effect2 = builder.evaluateReplace(rhs2Builder, mapping2);
  EXPECT_EQ(effect2.size, 1);
  EXPECT_EQ(effect2.depth, 0);
  builder.replace(rhs2Builder, mapping2);
  printBidirectCellsTrav(builder);

  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto &inputLinks = builder.addInputs(3);
    const auto &andLink0 = builder.addCell(model::AND, inputLinks[1],
                                           inputLinks[2]);
    const auto &andLink1 = builder.addCell(model::AND, inputLinks[0], andLink0);
    builder.addOutput(andLink1);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceBuilderTest, ReuseCellsFollowingRoot) {
  SubnetBuilder builder;
  const auto &inLinks = builder.addInputs(3);
  const auto &andLink0 = builder.addCell(model::AND, inLinks[0], inLinks[1]);
  const auto &andLink1 = builder.addCell(model::AND, inLinks[1], inLinks[2]);
  const auto &andLink2 = builder.addCell(model::AND, andLink0, andLink1);
  builder.addOutput(andLink2);
  const auto &orLink0 = builder.addCell(model::OR, inLinks[0], inLinks[1]);
  const auto &orLink1 = builder.addCell(model::OR, orLink0, inLinks[2]);
  const auto &orLink2 = builder.addCell(model::OR, orLink1, inLinks[2]);
  builder.addOutput(orLink2);

  SubnetBuilder rhsBuilder;
  const auto &rhsInLinks = rhsBuilder.addInputs(3);
  const auto &rhsOrLink0 = rhsBuilder.addCell(model::OR, rhsInLinks[0],
                                              rhsInLinks[1]);
  const auto &rhsOrLink1 = rhsBuilder.addCell(model::OR, rhsOrLink0,
                                              rhsInLinks[2]);
  const auto &rhsOrLink2 = rhsBuilder.addCell(model::OR, rhsOrLink1,
                                              rhsInLinks[2]);
  const auto &rhsBufLink0 = rhsBuilder.addCell(model::BUF, rhsOrLink2);
  rhsBuilder.addOutput(rhsBufLink0);

  InOutMapping mapping({0, 1, 2}, {5});

  const auto effect = builder.evaluateReplace(rhsBuilder, mapping);
  EXPECT_EQ(effect.size, 2);
  EXPECT_EQ(effect.depth, -2);
  builder.replace(rhsBuilder, mapping);
  printBidirectCellsTrav(builder);

  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto &inputLinks = builder.addInputs(3);
    const auto &orLink0 = builder.addCell(model::OR, inputLinks[0],
                                          inputLinks[1]);
    const auto &orLink1 = builder.addCell(model::OR, orLink0,
                                          inputLinks[2]);
    const auto &orLink2 = builder.addCell(model::OR, orLink1,
                                          inputLinks[2]);
    builder.addOutput(orLink2);
    const auto &bufLink0 = builder.addCell(model::BUF, orLink2);
    builder.addOutput(bufLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceBuilderTest, EvaluationTest) {
  SubnetBuilder builder;
  const auto &inLinks = builder.addInputs(3);
  const auto &andLink0 = builder.addCell(model::AND, inLinks[0], inLinks[1]);
  const auto &xorLink0 = builder.addCell(model::XOR, inLinks[1], inLinks[2]);
  const auto &orLink0 = builder.addCell(model::OR, andLink0, xorLink0);
  const auto &bufLink0 = builder.addCell(model::BUF, orLink0);
  const auto &bufLink1 = builder.addCell(model::BUF, ~orLink0);
  builder.addOutput(bufLink0);
  builder.addOutput(bufLink1);

  SubnetBuilder rhsBuilder;
  const auto &rhsInLinks = rhsBuilder.addInputs(3);
  const auto &rhsAndLink0 = rhsBuilder.addCell(model::AND, rhsInLinks[0],
                                               rhsInLinks[1]);
  const auto &rhsXorLink0 = rhsBuilder.addCell(model::XOR, rhsInLinks[1],
                                               inLinks[2]);
  const auto &rhsOrLink0 = rhsBuilder.addCell(model::OR, rhsAndLink0,
                                              rhsXorLink0);
  const auto &rhsBufLink0 = rhsBuilder.addCell(model::BUF, rhsOrLink0);
  rhsBuilder.addOutput(rhsBufLink0);

  InOutMapping mapping({0, 1, 2}, {6});

  const auto effect = builder.evaluateReplace(rhsBuilder, mapping);
  EXPECT_EQ(effect.size, 0);
  EXPECT_EQ(effect.depth, 0);
}

TEST(ReplaceBuilderTest, AddInvToPO) {
  SubnetBuilder builder;
  const auto &inLinks = builder.addInputs(2);
  const auto &andLink0 = builder.addCell(model::AND, inLinks[0], inLinks[1]);
  builder.addOutput(andLink0);

  SubnetBuilder rhsBuilder;
  const auto &rhsInLinks = rhsBuilder.addInputs(2);
  const auto &rhsAndLink0 = rhsBuilder.addCell(model::AND, rhsInLinks[0],
      rhsInLinks[1]);
  rhsBuilder.addOutput(~rhsAndLink0);

  InOutMapping mapping({0, 1}, {3});

  const auto effect = builder.evaluateReplace(rhsBuilder, mapping);
  EXPECT_EQ(effect.size, 0);
  EXPECT_EQ(effect.depth, 0);

  builder.replace(rhsBuilder, mapping);
  printBidirectCellsTrav(builder);

  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto &inputLinks = builder.addInputs(2);
    const auto &andLink0 = builder.addCell(model::AND, inputLinks[0],
                                          inputLinks[1]);
    builder.addOutput(~andLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceBuilderTest, POReplaceLarger) {
  SubnetBuilder builder;
  const auto &inLinks = builder.addInputs(2);
  const auto &andLink0 = builder.addCell(model::AND, inLinks[0], inLinks[1]);
  builder.addOutput(andLink0);

  SubnetBuilder rhsBuilder;
  const auto &rhsInLinks = rhsBuilder.addInputs(2);
  const auto &rhsBufLink0 = rhsBuilder.addCell(model::BUF, rhsInLinks[0]);
  const auto &rhsBufLink1 = rhsBuilder.addCell(model::BUF, rhsInLinks[1]);
  const auto &rhsAndLink0 = rhsBuilder.addCell(model::AND, rhsBufLink0,
      rhsBufLink1);
  rhsBuilder.addOutput(~rhsAndLink0);

  InOutMapping mapping({0, 1}, {3});

  const auto effect = builder.evaluateReplace(rhsBuilder, mapping);
  EXPECT_EQ(effect.size, -2);
  EXPECT_EQ(effect.depth, -1);

  builder.replace(rhsBuilder, mapping);
  printBidirectCellsTrav(builder);

  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto &inputLinks = builder.addInputs(2);
    const auto &bufLink0 = builder.addCell(model::BUF, inputLinks[0]);
    const auto &bufLink1 = builder.addCell(model::BUF, inputLinks[1]);
    const auto &andLink0 = builder.addCell(model::AND, bufLink0, bufLink1);
    builder.addOutput(~andLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceBuilderTest, POReplaceSmaller) {
  SubnetBuilder builder;
  const auto &inLinks = builder.addInputs(3);
  const auto &andLink0 = builder.addCell(model::AND, inLinks[0], inLinks[1]);
  const auto &andLink1 = builder.addCell(model::AND, inLinks[1], inLinks[2]);
  const auto &andLink2 = builder.addCell(model::AND, andLink0, andLink1);
  builder.addOutput(~andLink2);

  SubnetBuilder rhsBuilder;
  const auto &rhsInLinks = rhsBuilder.addInputs(3);
  const auto &rhsAndLink0 = rhsBuilder.addCell(model::AND, rhsInLinks[0],
      rhsInLinks[1], rhsInLinks[2]);
  rhsBuilder.addOutput(rhsAndLink0);

  InOutMapping mapping({0, 1, 2}, {6});

  const auto effect = builder.evaluateReplace(rhsBuilder, mapping);
  EXPECT_EQ(effect.size, 2);
  EXPECT_EQ(effect.depth, 1);

  builder.replace(rhsBuilder, mapping);
  printBidirectCellsTrav(builder);

  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto &inputLinks = builder.addInputs(3);
    const auto &andLink0 = builder.addCell(model::AND, inputLinks[0],
        inputLinks[1], inputLinks[2]);
    builder.addOutput(andLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceBuilderTest, InOutInvert) {
  SubnetBuilder builder;
  const auto &inLink = builder.addInput();
  builder.addOutput(inLink);

  SubnetBuilder rhsBuilder;
  const auto &rhsInLink = rhsBuilder.addInput();
  rhsBuilder.addOutput(~rhsInLink);

  InOutMapping mapping(EntryIDList{0}, EntryIDList{1});

  const auto effect = builder.evaluateReplace(rhsBuilder, mapping);
  EXPECT_EQ(effect.size, 0);
  EXPECT_EQ(effect.depth, 0);

  builder.replace(rhsBuilder, mapping);
  printBidirectCellsTrav(builder);

  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto &inputLink = builder.addInput();
    builder.addOutput(~inputLink);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceBuilderTest, POReplaceEqual) {
  SubnetBuilder builder;
  const auto &inLinks = builder.addInputs(2);
  const auto &andLink0 = builder.addCell(model::AND, inLinks[0], inLinks[1]);
  builder.addOutput(andLink0);

  SubnetBuilder rhsBuilder;
  const auto &rhsInLinks = rhsBuilder.addInputs(2);
  const auto &rhsAndLink0 = rhsBuilder.addCell(model::AND, rhsInLinks[0],
      rhsInLinks[1]);
  rhsBuilder.addOutput(rhsAndLink0);

  InOutMapping mapping({0, 1}, {3});

  const auto effect = builder.evaluateReplace(rhsBuilder, mapping);
  EXPECT_EQ(effect.size, 0);
  EXPECT_EQ(effect.depth, 0);

  builder.replace(rhsBuilder, mapping);
  printBidirectCellsTrav(builder);

  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto &inputLinks = builder.addInputs(2);
    const auto &andLink0 = builder.addCell(model::AND, inputLinks[0],
        inputLinks[1]);
    builder.addOutput(andLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceBuilderTest, POReplaceNotEqual) {
  SubnetBuilder builder;
  const auto &inLinks = builder.addInputs(2);
  const auto &andLink0 = builder.addCell(model::AND, inLinks[0], inLinks[1]);
  builder.addOutput(andLink0);

  SubnetBuilder rhsBuilder;
  const auto &rhsInLinks = rhsBuilder.addInputs(2);
  const auto &rhsOrLink0 = rhsBuilder.addCell(model::OR, rhsInLinks[0],
      rhsInLinks[1]);
  rhsBuilder.addOutput(rhsOrLink0);

  InOutMapping mapping({0, 1}, {3});

  const auto effect = builder.evaluateReplace(rhsBuilder, mapping);
  EXPECT_EQ(effect.size, 0);
  EXPECT_EQ(effect.depth, 0);

  builder.replace(rhsBuilder, mapping);
  printBidirectCellsTrav(builder);

  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);
  std::cout << result << '\n';

  {
    SubnetBuilder builder;
    const auto &inputLinks = builder.addInputs(2);
    const auto &andLink0 = builder.addCell(model::OR, inputLinks[0],
        inputLinks[1]);
    builder.addOutput(andLink0);
    subnetsEqual(resultID, builder.make());
  }
}

TEST(ReplaceBuilderTest, OneEntryTraversal) {
  SubnetBuilder builder;
  builder.addInput();
  printBidirectCellsTrav(builder);
}

TEST(ReplaceBuilderTest, WeightsEval) {
  SubnetBuilder builder;

  Subnet::LinkList inputs = builder.addInputs(3);
  Subnet::Link link1 = builder.addCell(AND, inputs[0], inputs[1]);
  Subnet::Link link2 = builder.addCell(BUF, inputs[2]);
  Subnet::Link link3 = builder.addCell(AND, link1, link2);
  builder.addOutput(link3);

  std::vector<float> weights{1, 1, 1, 1, 1, 1, 1};
  for (auto it = builder.begin(); it != builder.end(); ++it) {
    const EntryID j = *it;
    builder.setWeight(j, weights[j]);
  }

  /// RHS SubnetBuilder
  SubnetBuilder rhsBuilder;

  Subnet::LinkList rhsInputs = rhsBuilder.addInputs(3);
  Subnet::Link rhsLink1 = rhsBuilder.addCell(AND, rhsInputs[0], rhsInputs[1]);
  Subnet::Link rhsLink2 = rhsBuilder.addCell(BUF, rhsInputs[2]);
  Subnet::Link rhsLink3 = rhsBuilder.addCell(AND, rhsLink1, rhsInputs[1],
      rhsLink2);
  Subnet::Link rhsLink4 = rhsBuilder.addCell(BUF, rhsLink3);
  rhsBuilder.addOutput(rhsLink4);
  std::vector<float> rhsWeights{1, 1, 1, 1, 1, 1, 1, 1};
  for (auto it = rhsBuilder.begin(); it != rhsBuilder.end(); ++it) {
    const EntryID j = *it;
    rhsBuilder.setWeight(j, rhsWeights[j]);
  }

  InOutMapping mapping({0, 1, 2}, {5});
  SubnetBuilder::CellWeightModifier weightModifier =
      [](float p, uint16_t refcount) {
        return std::pow(2.f, refcount) + p;
  };

  const auto effect = builder.evaluateReplace(
    rhsBuilder, mapping, &weightModifier
  );

  EXPECT_TRUE(std::fabs(effect.weight + 5.f) < 1e-6);
}

TEST(ReplaceBuilderTest, WeightsEvalNewRoot) {
  SubnetBuilder builder;

  Subnet::LinkList inputs = builder.addInputs(2);
  Subnet::Link link1 = builder.addCell(AND, inputs[0], inputs[1]);
  Subnet::Link link2 = builder.addCell(AND, inputs[0], link1);
  Subnet::Link link3 = builder.addCell(AND, link1, inputs[1]);
  builder.addOutput(link2);
  builder.addOutput(link3);

  std::vector<float> weights{1, 1, 1, 1, 1, 1, 1};
  for (auto it = builder.begin(); it != builder.end(); ++it) {
    const EntryID j = *it;
    builder.setWeight(j, weights[j]);
  }

  /// RHS SubnetBuilder
  SubnetBuilder rhsBuilder;

  Subnet::LinkList rhsInputs = rhsBuilder.addInputs(2);
  Subnet::Link rhsLink1 = rhsBuilder.addCell(OR, rhsInputs[0], rhsInputs[1]);
  rhsBuilder.addOutput(rhsLink1);
  std::vector<float> rhsWeights{1, 1, 1, 1};
  for (auto it = rhsBuilder.begin(); it != rhsBuilder.end(); ++it) {
    const EntryID j = *it;
    rhsBuilder.setWeight(j, rhsWeights[j]);
  }

  InOutMapping mapping({0, 1}, {2});
  SubnetBuilder::CellWeightModifier weightModifier =
      [](float p, uint16_t refcount) {
        return std::pow(2.f, refcount) + p;
  };

  const auto effect = builder.evaluateReplace(
    rhsBuilder, mapping, &weightModifier
  );

  EXPECT_TRUE(std::fabs(effect.weight) < 1e-6);
}

} // namespace eda::gate::model
