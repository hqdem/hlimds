//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer2/rewriter.h"

#include "gtest/gtest.h"

namespace eda::gate::optimizer2 {

using ResynthesizerBase = optimizer2::ResynthesizerBase;
using Subnet = model::Subnet;
using LinkList = Subnet::LinkList;
using SubnetID = model::SubnetID;
using SubnetBuilder = model::SubnetBuilder;
using DinTruthTable = kitty::dynamic_truth_table;

class EqualResynthesizer : public ResynthesizerBase {
public:
  SubnetID resynthesize(const SubnetID subnetID) override {
    const Subnet &subnet = Subnet::get(subnetID);
    SubnetBuilder builder;
    const auto &inLinks = builder.addInputs(subnet.getInNum());
    const auto &outLinks = builder.addSubnet(subnetID, inLinks);
    builder.addOutputs(outLinks);
    return builder.make();
  }
};

class AddBufsResynthesizer : public ResynthesizerBase {
public:
  SubnetID resynthesize(const SubnetID subnetID) override {
    const Subnet &subnet = Subnet::get(subnetID);
    const auto &entries = subnet.getEntries();
    SubnetBuilder builder;
    LinkList newSubnetLinks;
    std::unordered_map<size_t, size_t> mapping;
    for (size_t i = 0; i < entries.size(); ++i) {
      const auto &cell = entries[i].cell;
      LinkList cellLinks;
      for (const auto &link : subnet.getLinks(i)) {
        size_t linkID;
        if (mapping.find(link.idx) != mapping.end()) {
          linkID = mapping[link.idx];
        } else {
          linkID = link.idx;
        }
        if (cell.isAnd()) {
          newSubnetLinks.push_back(builder.addCell(model::BUF,
                                                   ~newSubnetLinks[linkID]));
          linkID = newSubnetLinks.size() - 1;
        }
        cellLinks.push_back(newSubnetLinks[linkID]);
      }
      newSubnetLinks.push_back(builder.addCell(cell.getTypeID(), cellLinks));
      mapping[i] = newSubnetLinks.size() - 1;
      i += cell.more;
    }
    return builder.make();
  }
};

class DelBufsResynthesizer : public ResynthesizerBase {
public:
  SubnetID resynthesize(const SubnetID subnetID) override {
    const Subnet &subnet = Subnet::get(subnetID);
    const auto &entries = subnet.getEntries();
    SubnetBuilder builder;
    LinkList newSubnetLinks;
    std::unordered_map<size_t, size_t> mapping;
    for (size_t i = 0; i < entries.size(); ++i) {
      const auto &cell = entries[i].cell;
      LinkList cellLinks;
      for (const auto &link : subnet.getLinks(i)) {
        size_t linkID;
        if (mapping.find(link.idx) != mapping.end()) {
          linkID = mapping[link.idx];
        } else {
          linkID = link.idx;
        }
        cellLinks.push_back(newSubnetLinks[linkID]);
      }
      if (cell.isBuf()) {
        mapping[i] = cellLinks[0].idx;
        i += cell.more;
        continue;
      }
      newSubnetLinks.push_back(builder.addCell(cell.getTypeID(), cellLinks));
      mapping[i] = newSubnetLinks.size() - 1;
      i += cell.more;
    }
    return builder.make();
  }
};

bool truthTablesEqual(const SubnetID subnetID, const SubnetID targetSubnetID) {
  DinTruthTable t1 = evaluateSingleOut(Subnet::get(targetSubnetID));
  DinTruthTable t2 = evaluateSingleOut(Subnet::get(subnetID));
  return t1 == t2;
}

void runTest(
    ResynthesizerBase &resynthesizer,
    const SubnetID subnetID,
    const SubnetID targetSubnetID) {

  Rewriter rewriter;
  const auto &subnet = Subnet::get(subnetID);
  std::cout << "Before rewriting:\n" << subnet << '\n';

  SubnetBuilder builder;
  const auto &inputs = builder.addInputs(subnet.getInNum());
  const auto &links = builder.addSubnet(subnetID, inputs);
  builder.addOutputs(links);

  rewriter.rewrite(builder, resynthesizer, 5);
  const SubnetID newSubnetID = builder.make();
  std::cout << "After rewriting:\n" << Subnet::get(newSubnetID);

  ASSERT_TRUE(truthTablesEqual(newSubnetID, targetSubnetID));
}

SubnetID getNoBufsSubnet() {
  SubnetBuilder builder;
  const auto &links = builder.addInputs(3);
  const auto andLink = builder.addCell(model::AND, links[0], links[1],
                                       links[2]);
  builder.addOutput(andLink);
  return builder.make();
}

SubnetID getBufsSubnet2() {
  SubnetBuilder builder;
  const auto &links = builder.addInputs(5);
  const auto &bufLink0 = builder.addCell(model::BUF, ~links[0]);
  const auto &orLink0 = builder.addCell(model::OR, bufLink0, links[1]);
  const auto &andLink0 = builder.addCell(model::AND, links[1], links[2]);
  const auto &bufLink1 = builder.addCell(model::BUF, ~orLink0);
  const auto &bufLink2 = builder.addCell(model::BUF, ~andLink0);
  const auto &andLink1 = builder.addCell(model::AND, links[0], bufLink2);
  const auto &xorLink0 = builder.addCell(model::XOR, bufLink1, orLink0,
                                         andLink1, links[3], links[4]);
  const auto &bufLink3 = builder.addCell(model::BUF, ~xorLink0);
  builder.addOutput(bufLink3);
  return builder.make();
}

SubnetID getBufsSubnet() {
  SubnetBuilder builder;
  const auto &links = builder.addInputs(3);
  const auto &bufLink0 = builder.addCell(model::BUF, ~links[0]);
  const auto &bufLink1 = builder.addCell(model::BUF, ~links[1]);
  const auto &bufLink2 = builder.addCell(model::BUF, ~links[2]);
  const auto andLink = builder.addCell(model::AND, bufLink0, bufLink1,
                                       bufLink2);
  builder.addOutput(andLink);
  return builder.make();
}

TEST(RewriterTest, ReduceTest1) {
  DelBufsResynthesizer resynthesizer;
  const SubnetID subnetID = getNoBufsSubnet();
  runTest(resynthesizer, subnetID, getNoBufsSubnet());
}

TEST(RewriterTest, ReduceTest2) {
  DelBufsResynthesizer resynthesizer;
  const SubnetID subnetID = getBufsSubnet();
  runTest(resynthesizer, subnetID, getNoBufsSubnet());
}

TEST(RewriterTest, ReduceTest3) {
  DelBufsResynthesizer resynthesizer;
  const SubnetID subnetID = getBufsSubnet2();

  SubnetBuilder builder;
  const auto &inputLinks = builder.addInputs(5);
  const auto &orLink0 = builder.addCell(model::OR, inputLinks[0],
                                        inputLinks[1]);
  const auto &andLink0 = builder.addCell(model::AND, inputLinks[1],
                                         inputLinks[2]);
  const auto &andLink1 = builder.addCell(model::AND, inputLinks[0], andLink0);
  const auto &xorLink0 = builder.addCell(model::XOR, orLink0, orLink0,
                                         andLink1, inputLinks[3],
                                         inputLinks[4]);
  builder.addOutput(xorLink0);

  runTest(resynthesizer, subnetID, builder.make());
}

TEST(RewriterTest, EnlargeTest1) {
  AddBufsResynthesizer resynthesizer;
  const SubnetID subnetID = getNoBufsSubnet();

  runTest(resynthesizer, subnetID, getNoBufsSubnet());
}

TEST(RewriterTest, EnlargeTest2) {
  AddBufsResynthesizer resynthesizer;
  const SubnetID subnetID = getBufsSubnet2();
  runTest(resynthesizer, subnetID, getBufsSubnet2());
}

TEST(RewriterTest, EqualTest1) {
  EqualResynthesizer resynthesizer;
  const SubnetID subnetID = getNoBufsSubnet();
  runTest(resynthesizer, subnetID, getNoBufsSubnet());
}

TEST(RewriterTest, EqualTest2) {
  EqualResynthesizer resynthesizer;
  const SubnetID subnetID = getBufsSubnet2();
  runTest(resynthesizer, subnetID, getBufsSubnet2());
}

} // namespace eda::gate::optimizer2
