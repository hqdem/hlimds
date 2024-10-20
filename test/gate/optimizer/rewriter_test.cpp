//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/utils/subnet_truth_table.h"
#include "gate/optimizer/rewriter.h"

#include "gtest/gtest.h"

namespace eda::gate::optimizer {

using ResynthesizerBase = optimizer::ResynthesizerBase;
using Subnet = model::Subnet;
using LinkList = Subnet::LinkList;
using SubnetID = model::SubnetID;
using SubnetBuilder = model::SubnetBuilder;
using SubnetObject = model::SubnetObject;
using SubnetView = model::SubnetView;
using Effect = SubnetBuilder::Effect;
using TruthTable = model::TruthTable;
using EntryID = model::EntryID;

static SubnetID getSubnetID(const SubnetView &window) {
  auto &subnetWindow = const_cast<SubnetView&>(window);
  return subnetWindow.getSubnet().make();
}

static const Subnet &getSubnet(const SubnetView &window) {
  auto &subnetWindow = const_cast<SubnetView&>(window);
  return subnetWindow.getSubnet().makeObject();
}

class EqualResynthesizer : public ResynthesizerBase {
public:
  SubnetObject resynthesize(const SubnetView &window,
                            uint16_t) const override {
    return SubnetObject{getSubnetID(window)};
  }
};

class AddBufsResynthesizer : public ResynthesizerBase {
public:
  SubnetObject resynthesize(const SubnetView &window,
                            uint16_t) const override {
    const Subnet &oldSubnet = getSubnet(window);
    const auto &entries = oldSubnet.getEntries();
    SubnetBuilder newSubnetBuilder;
    LinkList newSubnetLinks;
    std::unordered_map<EntryID, EntryID> linkMapping;
    for (size_t i = 0; i < entries.size(); ++i) {
      const auto &cell = entries[i].cell;
      LinkList cellLinks;
      for (const auto &link : oldSubnet.getLinks(i)) {
        EntryID linkID;
        if (linkMapping.find(link.idx) != linkMapping.end()) {
          linkID = linkMapping[link.idx];
        } else {
          linkID = link.idx;
        }
        if (cell.isAnd()) {
          newSubnetLinks.push_back(
              newSubnetBuilder.addCell(model::BUF, ~newSubnetLinks[linkID]));
          linkID = newSubnetLinks.size() - 1;
        }
        cellLinks.push_back(newSubnetLinks[linkID]);
      }
      newSubnetLinks.push_back(
          newSubnetBuilder.addCell(cell.getTypeID(), cellLinks));
      linkMapping[i] = newSubnetLinks.size() - 1;
      i += cell.more;
    }
    return SubnetObject{newSubnetBuilder.make()};
  }
};

class DelBufsResynthesizer : public ResynthesizerBase {
public:
  SubnetObject resynthesize(const SubnetView &window,
                            uint16_t) const override {
    const Subnet &oldSubnet = getSubnet(window);
    const auto &entries = oldSubnet.getEntries();
    SubnetBuilder newSubnetBuilder;
    LinkList newSubnetLinks;
    std::unordered_map<EntryID, EntryID> linkMapping;
    for (size_t i = 0; i < entries.size(); ++i) {
      const auto &cell = entries[i].cell;
      LinkList cellLinks;
      for (const auto &link : oldSubnet.getLinks(i)) {
        EntryID linkID;
        if (linkMapping.find(link.idx) != linkMapping.end()) {
          linkID = linkMapping[link.idx];
        } else {
          linkID = link.idx;
        }
        cellLinks.push_back(newSubnetLinks[linkID]);
      }
      if (cell.isBuf()) {
        linkMapping[i] = cellLinks[0].idx;
        i += cell.more;
        continue;
      }
      newSubnetLinks.push_back(
          newSubnetBuilder.addCell(cell.getTypeID(), cellLinks));
      linkMapping[i] = newSubnetLinks.size() - 1;
      i += cell.more;
    }
    return SubnetObject{newSubnetBuilder.make()};
  }
};

bool truthTablesEqual(const SubnetID subnetID, const SubnetID targetSubnetID) {
  TruthTable t1 = model::evaluateSingleOut(Subnet::get(targetSubnetID));
  TruthTable t2 = model::evaluateSingleOut(Subnet::get(subnetID));
  return t1 == t2;
}

void runTest(
    const ResynthesizerBase &resynthesizer,
    const SubnetID subnetID,
    const SubnetID targetSubnetID) {

  Rewriter rewriter("rw", resynthesizer, 5, [](const Effect &effect) -> float {
    return (float)effect.size; });
  const auto &subnet = Subnet::get(subnetID);
  std::cout << "Before rewriting:\n" << subnet << '\n';

  auto builder = std::make_shared<SubnetBuilder>();
  const auto &inputs = builder->addInputs(subnet.getInNum());
  const auto &links = builder->addSubnet(subnetID, inputs);
  builder->addOutputs(links);

  rewriter.transform(builder);
  const auto newSubnetID = builder->make();
  std::cout << "After rewriting:\n" << Subnet::get(newSubnetID);

  ASSERT_TRUE(truthTablesEqual(newSubnetID, targetSubnetID));
}

SubnetID getNoBufsSubnet() {
  auto builder = std::make_shared<SubnetBuilder>();
  const auto &links = builder->addInputs(3);
  const auto andLink = builder->addCell(model::AND, links[0], links[1],
                                       links[2]);
  builder->addOutput(andLink);
  return builder->make();
}

SubnetID getBufsSubnet2() {
  auto builder = std::make_shared<SubnetBuilder>();
  const auto &links = builder->addInputs(3);
  const auto &bufLink0 = builder->addCell(model::BUF, ~links[0]);
  const auto &orLink0 = builder->addCell(model::OR, bufLink0, links[1]);
  const auto &andLink0 = builder->addCell(model::AND, links[1], links[2]);
  const auto &bufLink1 = builder->addCell(model::BUF, ~orLink0);
  const auto &bufLink2 = builder->addCell(model::BUF, ~andLink0);
  const auto &andLink1 = builder->addCell(model::AND, links[0], bufLink2);
  const auto &xorLink0 = builder->addCell(model::XOR, bufLink1, orLink0,
                                          andLink1);
  const auto &bufLink3 = builder->addCell(model::BUF, ~xorLink0);
  builder->addOutput(bufLink3);
  return builder->make();
}

SubnetID getBufsSubnet() {
  auto builder = std::make_shared<SubnetBuilder>();
  const auto &links = builder->addInputs(3);
  const auto &bufLink0 = builder->addCell(model::BUF, ~links[0]);
  const auto &bufLink1 = builder->addCell(model::BUF, ~links[1]);
  const auto &bufLink2 = builder->addCell(model::BUF, ~links[2]);
  const auto andLink = builder->addCell(model::AND, bufLink0, bufLink1,
                                       bufLink2);
  builder->addOutput(andLink);
  return builder->make();
}

TEST(RewriterTest, ReduceTest1) {
  const DelBufsResynthesizer resynthesizer;
  const SubnetID subnetID = getNoBufsSubnet();
  runTest(resynthesizer, subnetID, getNoBufsSubnet());
}

TEST(RewriterTest, ReduceTest2) {
  const DelBufsResynthesizer resynthesizer;
  const SubnetID subnetID = getBufsSubnet();
  runTest(resynthesizer, subnetID, getNoBufsSubnet());
}

TEST(RewriterTest, ReduceTest3) {
  const DelBufsResynthesizer resynthesizer;
  const SubnetID subnetID = getBufsSubnet2();

  auto builder = std::make_shared<SubnetBuilder>();
  const auto &inputLinks = builder->addInputs(3);
  const auto &orLink0 = builder->addCell(model::OR, inputLinks[0],
                                         inputLinks[1]);
  const auto &andLink0 = builder->addCell(model::AND, inputLinks[1],
                                          inputLinks[2]);
  const auto &andLink1 = builder->addCell(model::AND, inputLinks[0], andLink0);
  const auto &xorLink0 = builder->addCell(model::XOR, orLink0, orLink0,
                                          andLink1);
  builder->addOutput(xorLink0);

  runTest(resynthesizer, subnetID, builder->make());
}

TEST(RewriterTest, EnlargeTest1) {
  const AddBufsResynthesizer resynthesizer;
  const SubnetID subnetID = getNoBufsSubnet();
  runTest(resynthesizer, subnetID, getNoBufsSubnet());
}

TEST(RewriterTest, EnlargeTest2) {
  const AddBufsResynthesizer resynthesizer;
  const SubnetID subnetID = getBufsSubnet2();
  runTest(resynthesizer, subnetID, getBufsSubnet2());
}

TEST(RewriterTest, EqualTest1) {
  const EqualResynthesizer resynthesizer;
  const SubnetID subnetID = getNoBufsSubnet();
  runTest(resynthesizer, subnetID, getNoBufsSubnet());
}

TEST(RewriterTest, EqualTest2) {
  const EqualResynthesizer resynthesizer;
  const SubnetID subnetID = getBufsSubnet2();
  runTest(resynthesizer, subnetID, getBufsSubnet2());
}

} // namespace eda::gate::optimizer
