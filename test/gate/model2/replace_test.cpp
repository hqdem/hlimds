//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021-2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/subnet.h"

#include "gtest/gtest.h"

#include <iostream>
#include <unordered_map>

namespace eda::gate::model {

template<typename IterT>
void printCellsTrav(SubnetBuilder &builder, IterT it, IterT contaiterEnd) {
  for (; it != contaiterEnd; ++it) {
    const auto &cell = builder.getEntry(*it).cell;
    std::cout << "Current entry ID: " << *it << "; input entries IDs: ";
    for (std::size_t i = 0; i < cell.arity; ++i) {
      std::cout << cell.link[i].idx << ' ';
    }
    std::cout << '\n';
  }
  std::cout << '\n';
}

void printBidirectCellsTrav(SubnetBuilder &builder) {
  std::cout << "Forward entries traversal:\n";
  printCellsTrav(builder, builder.begin(), builder.end());
  std::cout << "Reverse entries traversal:\n";
  printCellsTrav(builder, builder.rbegin(), builder.rend());
}

bool linksEqual(const Subnet::Link &targetLink, const Subnet::Link &srcLink) {
  return targetLink.idx == srcLink.idx &&
         targetLink.out == srcLink.out &&
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
  for (std::size_t i = 0; i < targetCell.arity; ++i) {
    if (!linksEqual(targetCell.link[i], srcCell.link[i])) {
      return false;
    }
  }
  return true;
}

void subnetsEqual(const SubnetID target, const SubnetID src) {
  const auto &targetEntries = Subnet::get(target).getEntries();
  const auto &srcEntries = Subnet::get(src).getEntries();
  EXPECT_TRUE(targetEntries.size() == srcEntries.size());
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
  std::unordered_map<std::size_t, std::size_t> mapping;
  mapping[0] = 0;
  mapping[1] = 1;
  mapping[4] = 2;

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
  std::unordered_map<std::size_t, std::size_t> mapping;
  mapping[0] = 0;
  mapping[1] = 1;
  mapping[2] = 2;
  mapping[4] = 5;

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
  std::unordered_map<std::size_t, std::size_t> mapping;
  mapping[0] = 0;
  mapping[1] = 1;
  mapping[2] = 2;
  mapping[8] = 5;

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
  std::unordered_map<std::size_t, std::size_t> mapping;
  mapping[0] = 3;
  mapping[1] = 4;
  mapping[3] = 5;

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
  std::unordered_map<std::size_t, std::size_t> mapping;
  mapping[0] = 0;
  mapping[1] = 1;
  mapping[2] = 2;
  mapping[7] = 5;

  builder.replace(rhsID, mapping);
  printBidirectCellsTrav(builder);

  SubnetBuilder rhsBuilder2;
  const auto rhs2Inputs = rhsBuilder2.addInputs(1);
  const auto rhs2BufLink0 = rhsBuilder2.addCell(model::BUF, rhs2Inputs[0]);
  const auto rhs2BufLink1 = rhsBuilder2.addCell(model::BUF, rhs2BufLink0);
  rhsBuilder2.addOutput(rhs2BufLink1);

  const auto rhs2ID = rhsBuilder2.make();
  std::unordered_map<std::size_t, std::size_t> mapping2;
  mapping2[0] = 0;
  mapping2[3] = 7;

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
  std::unordered_map<std::size_t, std::size_t> mapping;
  mapping[1] = 3;

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
  std::unordered_map<std::size_t, std::size_t> mapping;
  mapping[0] = inputs[1].idx;
  mapping[1] = inputs[2].idx;
  mapping[2] = inputs[3].idx;
  mapping[4] = orLink1.idx;

  builder.replace(rhsID, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(builder.make());
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
  const auto BufLink0 = builder.addCell(model::BUF, inputs[0]);
  const auto BufLink1 = builder.addCell(model::BUF, inputs[1]);
  const auto BufLink2 = builder.addCell(model::BUF, inputs[1]);
  const auto BufLink3 = builder.addCell(model::BUF, inputs[2]);
  const auto xorLink0 = builder.addCell(model::XOR, BufLink0, BufLink1,
                                        BufLink2, BufLink3);
  builder.addOutput(xorLink0);

  SubnetBuilder rhsBuilder;
  const auto rhsInputs = rhsBuilder.addInputs(3);
  const auto rhsXorLink0 = rhsBuilder.addCell(model::XOR, rhsInputs[0],
                                              rhsInputs[1], rhsInputs[2]);
  rhsBuilder.addOutput(rhsXorLink0);

  const auto rhsID = rhsBuilder.make();
  std::unordered_map<std::size_t, std::size_t> mapping;
  mapping[0] = inputs[0].idx;
  mapping[1] = inputs[1].idx;
  mapping[2] = inputs[2].idx;
  mapping[4] = xorLink0.idx;

  builder.replace(rhsID, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(builder.make());
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
  const auto rhsXorLink0 = rhsBuilder.addCell(model::XOR,
                                              Subnet::Link(rhsInputs[0].idx, 1),
                                              rhsInputs[1]);
  rhsBuilder.addOutput(rhsXorLink0);

  const auto rhsID = rhsBuilder.make();
  std::unordered_map<std::size_t, std::size_t> mapping;
  mapping[0] = inputs[0].idx;
  mapping[1] = inputs[1].idx;
  mapping[3] = xorLink0.idx;

  builder.replace(rhsID, mapping);
  printBidirectCellsTrav(builder);
  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(builder.make());
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
  std::unordered_map<std::size_t, std::size_t> mapping;
  mapping[0] = inputs[0].idx;
  mapping[1] = inputs[1].idx;
  mapping[5] = andLink0.idx;

  builder.replace(rhsID, mapping);
  printBidirectCellsTrav(builder);
  const auto bufLink0 = builder.addCell(model::BUF, andLink0);
  builder.addOutput(bufLink0);

  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(builder.make());
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

TEST(ReplaceTest, OneEntryTraversal) {
  SubnetBuilder builder;
  builder.addInput();
  printBidirectCellsTrav(builder);
}

} // namespace eda::gate::model
