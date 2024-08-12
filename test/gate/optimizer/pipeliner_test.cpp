//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/generator/matrix_generator.h"
#include "gate/model/design.h"
#include "gate/optimizer/pipeliner.h"

#include "gtest/gtest.h"

namespace eda::gate::optimizer {

using SubnetID = model::SubnetID;
using SubnetBuilder = model::SubnetBuilder;
using SubnetObject = model::SubnetObject;
using CellSymbol = model::CellSymbol;
using CellTypeID = model::CellTypeID;
using PhysicalProperties = model::PhysicalProperties;
using MatrixGenerator = model::MatrixGenerator;
using NetID = model::NetID;
using DesignBuilder = model::DesignBuilder;
using SubnetMarkup = Pipeliner::SubnetMarkup;

static size_t curCellID = 1;

static bool markupsEqual(
    const SubnetMarkup &lhs,
    const SubnetMarkup &rhs) {
  return lhs.markedLinks == rhs.markedLinks;
}

static bool checkTriggersEachPath(
    const SubnetBuilderPtr &builder,
    const SubnetMarkup &subnetMarkup,
    const size_t k,
    const size_t entryID,
    const size_t triggersN = 0) {

  const auto &cell = builder->getCell(entryID);
  if (cell.isIn() || cell.isZero() || cell.isOne()) {
    return triggersN == k - 1;
  }
  const auto &links = builder->getLinks(entryID);
  for (size_t i = 0; i < links.size(); ++i) {
    if (!checkTriggersEachPath(builder, subnetMarkup, k, links[i].idx,
        triggersN + subnetMarkup.getTriggersN(entryID, i))) {

      return false;
    }
  }
  return true;
}

static void testTriggersN(
    const SubnetBuilderPtr &builder,
    const SubnetMarkup &markedLinks,
    const size_t k) {

  bool correctTriggersN = true;
  for (auto it = builder->rbegin(); it != builder->rend(); ++it) {
    const size_t entryID = *it;
    if (!builder->getCell(entryID).isOut()) {
      break;
    }
    if (!checkTriggersEachPath(builder, markedLinks, k, entryID)) {
      correctTriggersN = false;
    }
  }
  EXPECT_TRUE(correctTriggersN);
}

static void test(
    const SubnetBuilderPtr &builder,
    const size_t k,
    const SubnetMarkup &correctSubnetMarkup) {

  Pipeliner pipeliner(k);
  auto subnetMarkup = pipeliner.markCascades(builder);

  // const auto &marked = subnetMarkup.markedLinks;
  // for (std::size_t i = 0; i < marked.size(); ++i) {
  //   if (marked[i].empty()) {
  //     std::cout << "{}\n";
  //     continue;
  //   }
  //   std::cout << "{";
  //   for (const auto elem : marked[i]) {
  //     std::cout << elem << ' ';
  //   }
  //   std::cout << "}\n";
  // }
  // std::cout << '\n';

  testTriggersN(builder, subnetMarkup, k);

  EXPECT_TRUE(markupsEqual(subnetMarkup, correctSubnetMarkup));
}

CellTypeID makeTmpType(
    const CellSymbol symbol,
    const PhysicalProperties &physProps,
    const size_t nIn = 1,
    const size_t nOut = 1) {
  const auto attrID = makeCellTypeAttr({1}, {1}, physProps);
  const model::CellProperties props(1, 1, 0, 0, 0, 0, 0, 0, 0);
  std::string name = "tmp_" + std::to_string(curCellID);
  ++curCellID;
  const auto customTypeID = model::makeCellType(
    symbol, name, model::OBJ_NULL_ID, attrID, props, nIn, nOut
  );
  return customTypeID;
}

SubnetBuilderPtr getOneBufBuilder() {
  auto bufPhysProps = PhysicalProperties{0.f, 1.f, 0.f};
  const auto bufTypeID = makeTmpType(model::BUF, bufPhysProps);

  auto builder = std::make_shared<SubnetBuilder>();
  const auto &input = builder->addInput();
  const auto &bufLink1 = builder->addCell(bufTypeID, {input});
  builder->addOutput(bufLink1);
  return builder;
}

TEST(PipelinerTest, TriggerAfterPI) {
  auto builder = getOneBufBuilder();
  test(builder, 2, SubnetMarkup({{}, {}, {1}}));
}

TEST(PipelinerTest, TriggersSequence) {
  auto builder = getOneBufBuilder();
  test(builder, 10, SubnetMarkup({{}, {1}, {8}}));
}

TEST(PipelinerTest, OneCascade) {
  auto andPhysProps = PhysicalProperties{0.f, 1.f, 0.f};
  const auto andTypeID = makeTmpType(model::AND, andPhysProps);

  auto builder = std::make_shared<SubnetBuilder>();
  const auto &inputs = builder->addInputs(2);
  const auto &andLink1 = builder->addCell(andTypeID, {inputs[0], inputs[1]});
  builder->addOutput(andLink1);
  test(builder, 1, SubnetMarkup({{}, {}, {}, {}}));
}

TEST(PipelinerTest, InOut) {
  auto builder = std::make_shared<SubnetBuilder>();
  const auto &input = builder->addInput();
  builder->addOutput(input);
  test(builder, 2, SubnetMarkup({{}, {1}}));
}

TEST(PipelinerTest, MinimalPartition) {
  auto bufPhysProps = PhysicalProperties{0.f, 1.f, 0.f};
  const auto bufTypeID = makeTmpType(model::BUF, bufPhysProps);

  auto builder = std::make_shared<SubnetBuilder>();
  const auto &input = builder->addInput();
  const auto &bufLink1 = builder->addCell(bufTypeID, {input});
  const auto &bufLink2 = builder->addCell(bufTypeID, {bufLink1});
  const auto &bufLink3 = builder->addCell(bufTypeID, {bufLink2});
  builder->addOutput(bufLink3);
  test(builder, 3, SubnetMarkup({{}, {}, {1}, {1}, {}}));
}

TEST(PipelinerTest, DifferentDelays) {
  auto bufPhysProps1 = PhysicalProperties{0.f, 0.5f, 0.f};
  auto bufPhysProps2 = PhysicalProperties{0.f, 1.f, 0.f};
  const auto bufTypeID1 = makeTmpType(model::BUF, bufPhysProps1);
  const auto bufTypeID2 = makeTmpType(model::BUF, bufPhysProps2);

  auto builder = std::make_shared<SubnetBuilder>();
  const auto &input = builder->addInput();
  const auto &bufLink1 = builder->addCell(bufTypeID1, {input});
  const auto &bufLink2 = builder->addCell(bufTypeID1, {bufLink1});
  const auto &bufLink3 = builder->addCell(bufTypeID2, {bufLink2});
  const auto &bufLink4 = builder->addCell(bufTypeID2, {bufLink3});
  const auto &bufLink5 = builder->addCell(bufTypeID1, {bufLink4});
  const auto &bufLink6 = builder->addCell(bufTypeID1, {bufLink5});
  builder->addOutput(bufLink6);
  test(builder, 3, SubnetMarkup({{}, {}, {}, {1}, {}, {1}, {}, {}}));
}

TEST(PipelinerTest, DiffPaths) {
  auto bufPhysProps1 = PhysicalProperties{0.f, 0.5f, 0.f};
  auto bufPhysProps2 = PhysicalProperties{0.f, 0.51f, 0.f};
  auto bufPhysProps3 = PhysicalProperties{0.f, 0.8f, 0.f};
  auto andPhysProps1 = PhysicalProperties{0.f, 0.9f, 0.f};
  auto andPhysProps2 = PhysicalProperties{0.f, 0.52f, 0.f};
  const auto bufTypeID1 = makeTmpType(model::BUF, bufPhysProps1);
  const auto bufTypeID2 = makeTmpType(model::BUF, bufPhysProps2);
  const auto bufTypeID3 = makeTmpType(model::BUF, bufPhysProps3);
  const auto andTypeID1 = makeTmpType(model::AND, andPhysProps1);
  const auto andTypeID2 = makeTmpType(model::AND, andPhysProps2);

  auto builder = std::make_shared<SubnetBuilder>();
  const auto &inputs = builder->addInputs(3);
  const auto &bufLink1 = builder->addCell(bufTypeID1, {inputs[0]});
  const auto &bufLink2 = builder->addCell(bufTypeID2, {bufLink1});
  const auto &bufLink3 = builder->addCell(bufTypeID3, {inputs[1]});
  const auto &andLink1 = builder->addCell(andTypeID1, {bufLink3, inputs[2]});
  const auto &andLink2 = builder->addCell(andTypeID2, {bufLink2, bufLink3});
  builder->addOutput(andLink2);
  builder->addOutput(andLink1);
  test(builder, 3,
       SubnetMarkup({{}, {}, {}, {}, {1}, {1}, {1, 2}, {1, 1}, {}, {}}));
}

TEST(PipelinerTest, IntersectingLayers) {
  auto bufPhysProps1 = PhysicalProperties{0.f, 1.f, 0.f};
  auto bufPhysProps2 = PhysicalProperties{0.f, 2.f, 0.f};
  auto bufPhysProps3 = PhysicalProperties{0.f, 3.f, 0.f};
  auto bufPhysProps4 = PhysicalProperties{0.f, 10.f, 0.f};
  const auto bufTypeID1 = makeTmpType(model::BUF, bufPhysProps1);
  const auto bufTypeID2 = makeTmpType(model::BUF, bufPhysProps2);
  const auto bufTypeID3 = makeTmpType(model::BUF, bufPhysProps3);
  const auto bufTypeID4 = makeTmpType(model::BUF, bufPhysProps4);

  auto builder = std::make_shared<SubnetBuilder>();
  const auto &input = builder->addInput();
  const auto &bufLink1 = builder->addCell(bufTypeID1, {input});
  const auto &bufLink2 = builder->addCell(bufTypeID2, {bufLink1});
  const auto &bufLink3 = builder->addCell(bufTypeID3, {bufLink2});
  builder->addOutput(bufLink3);
  const auto &bufLink4 = builder->addCell(bufTypeID1, {~input});
  const auto &bufLink5 = builder->addCell(bufTypeID1, {bufLink4});
  const auto &bufLink6 = builder->addCell(bufTypeID4, {bufLink5});
  const auto &bufLink7 = builder->addCell(bufTypeID1, {bufLink6});
  const auto &bufLink8 = builder->addCell(bufTypeID1, {bufLink7});
  builder->addOutput(bufLink8);

  test(builder, 2,
       SubnetMarkup({{}, {}, {}, {1}, {}, {}, {}, {1}, {}, {}, {}}));
}

TEST(PipelinerTest, DiffPathsSameBeginEnd) {
  auto physProps = PhysicalProperties{0.f, 1.f, 0.f};
  const auto bufTypeID1 = makeTmpType(model::BUF, physProps);
  const auto andTypeID1 = makeTmpType(model::AND, physProps);

  auto builder = std::make_shared<SubnetBuilder>();
  const auto &input = builder->addInput();
  const auto &bufLink1 = builder->addCell(bufTypeID1, {input});
  const auto &andLink1 = builder->addCell(andTypeID1, {bufLink1, input});
  const auto &bufLink2 = builder->addCell(bufTypeID1, {andLink1});
  const auto &bufLink3 = builder->addCell(bufTypeID1, {bufLink2});
  const auto &andLink2 = builder->addCell(andTypeID1, {bufLink2, bufLink3});
  builder->addOutput(andLink2);
  test(builder, 3, SubnetMarkup({{}, {}, {}, {1}, {}, {1, 1}, {}}));
}

TEST(PipelinerTest, TriggersInCascade) {
  auto bufPhysProps1 = PhysicalProperties{0.f, 1.f, 0.f};
  auto bufPhysProps2 = PhysicalProperties{0.f, 0.5f, 0.f};
  const auto bufTypeID1 = makeTmpType(model::BUF, bufPhysProps1);
  const auto bufTypeID2 = makeTmpType(model::BUF, bufPhysProps2);

  auto builder = std::make_shared<SubnetBuilder>();
  const auto &input = builder->addInput();
  const auto &bufLink1 = builder->addCell(bufTypeID1, {input});
  builder->addOutput(input);
  builder->addOutput(bufLink1);
  const auto &bufLink2 = builder->addCell(bufTypeID2, {bufLink1});
  builder->addOutput(bufLink2);
  const auto &bufLink3 = builder->addCell(bufTypeID2, {bufLink2});
  builder->addOutput(bufLink3);
  test(builder, 3, SubnetMarkup({{}, {}, {2}, {2}, {1}, {1}, {1}, {}}));
}

TEST(PipelinerTest, RandomSubnet) {
  auto physProps1 = PhysicalProperties{0.f, 0.2f, 0.f};
  auto physProps2 = PhysicalProperties{0.f, 0.3f, 0.f};
  auto physProps3 = PhysicalProperties{0.f, 0.4f, 0.f};
  auto physProps4 = PhysicalProperties{0.f, 0.401f, 0.f};
  auto physProps5 = PhysicalProperties{0.f, 0.40101f, 0.f};
  auto physProps6 = PhysicalProperties{0.f, 0.42f, 0.f};
  const auto andTypeID1 = makeTmpType(model::AND, physProps1, 5, 40);
  const auto andTypeID2 = makeTmpType(model::AND, physProps2, 5, 40);
  const auto andTypeID3 = makeTmpType(model::AND, physProps3, 5, 40);
  const auto andTypeID4 = makeTmpType(model::AND, physProps4, 5, 40);
  const auto andTypeID5 = makeTmpType(model::AND, physProps5, 5, 40);
  const auto bufTypeID1 = makeTmpType(model::BUF, physProps6, 1, 40);

  MatrixGenerator generator(
    2500, 5000, 3000,
    {andTypeID1, andTypeID2, andTypeID3, andTypeID4, andTypeID5, bufTypeID1},
    (unsigned)0
  );
  generator.setFaninHigh(5);
  NetID netID = generator.generate();
  assert(netID != model::OBJ_NULL_ID);
  DesignBuilder designBuilder(netID);
  SubnetID subnetID = designBuilder.getSubnetID(0);
  assert(subnetID != model::OBJ_NULL_ID);

  auto builder = std::make_shared<SubnetBuilder>(subnetID);
  size_t k = 15;
  Pipeliner pipeliner(k);
  const auto markedLinks = pipeliner.markCascades(builder);
  testTriggersN(builder, markedLinks, k);
}

} // namespace eda::gate::optimizer
