//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer2/cone_builder.h"

#include "gtest/gtest.h"

namespace eda::gate::optimizer2 {

using Subnet = model::Subnet;
using SubnetID = model::SubnetID;
using SubnetBuilder = model::SubnetBuilder;
using Link = model::Subnet::Link;
using Cut = CutExtractor::Cut;
using Cone = ConeBuilder::Cone;

bool coneValid(const Subnet &subnet,
               Cone cone,
               const uint64_t coneEntryIdx,
               const bool isMaxCone) {

  if (cone.coneEntryToOrig.find(coneEntryIdx) == cone.coneEntryToOrig.end()) {
    return false;
  }
  const uint64_t subnetEntryIdx = cone.coneEntryToOrig[coneEntryIdx];
  const Subnet &coneSubnet = Subnet::get(cone.subnetID);
  const auto &subnetCell = subnet.getEntries()[subnetEntryIdx].cell;
  const auto &coneCell = coneSubnet.getEntries()[coneEntryIdx].cell;
  if (subnetCell.getSymbol() != coneCell.getSymbol()) {
    return false;
  }
  if (!coneCell.isIn() && subnet.getLinks(subnetEntryIdx).size() !=
      coneSubnet.getLinks(coneEntryIdx).size()) {
    return false;
  }
  if (coneCell.isIn() && isMaxCone && !subnetCell.isIn()) {
    return false;
  }
  for (const auto inputEntry : coneSubnet.getLinks(coneEntryIdx)) {
    if (!coneValid(subnet, cone, inputEntry.idx, isMaxCone)) {
      return false;
    }
  }
  return true;
}

void cutConesValid(const Subnet &subnet,
                   const CutExtractor &cutExtractor,
                   const ConeBuilder &coneBuilder) {

  if (!getenv("UTOPIA_HOME")) {
    FAIL() << "UTOPIA_HOME is not set.";
  }
  const auto &subnetEntries = subnet.getEntries();
  for (uint64_t entryIdx = 0; entryIdx < subnetEntries.size(); ++entryIdx) {
    const auto &cuts = cutExtractor.getCuts(entryIdx);
    for (const auto &cut : cuts) {
      const Cone cone = coneBuilder.getCone(cut);
      const auto &coneSubnetEntries = Subnet::get(cone.subnetID).getEntries();
      EXPECT_TRUE(coneValid(subnet, cone, coneSubnetEntries.size() - 1, false));
    }
    entryIdx += subnetEntries[entryIdx].cell.more;
  }
}

void maxConesValid(const Subnet &subnet,
                   const ConeBuilder &coneBuilder) {

  if (!getenv("UTOPIA_HOME")) {
    FAIL() << "UTOPIA_HOME is not set.";
  }
  const auto &subnetEntries = subnet.getEntries();
  for (uint64_t entryIdx = 0; entryIdx < subnetEntries.size(); ++entryIdx) {
    const Cone cone = coneBuilder.getMaxCone(entryIdx);
    const auto &coneSubnetEntries = Subnet::get(cone.subnetID).getEntries();
    EXPECT_TRUE(coneValid(subnet, cone, coneSubnetEntries.size() - 1, true));
    entryIdx += subnetEntries[entryIdx].cell.more;
  }
}

TEST(ConeBuilderTest, SimpleTest) {
  SubnetBuilder builder;

  std::size_t inputIdx0 = builder.addCell(model::IN, SubnetBuilder::INPUT);
  std::size_t inputIdx1 = builder.addCell(model::IN, SubnetBuilder::INPUT);
  std::size_t andIdx0 = builder.addCell(model::AND, inputIdx0, inputIdx1);
  builder.addCell(model::OUT, Link(andIdx0), SubnetBuilder::OUTPUT);
  Subnet subnet = Subnet::get(builder.make());

  CutExtractor cutExtractor(&subnet, 5);
  ConeBuilder coneBuilder(&subnet);

  cutConesValid(subnet, cutExtractor, coneBuilder);
}

TEST(ConeBuilderTest, OneElementCut) {
  SubnetBuilder builder;

  builder.addCell(model::OUT, SubnetBuilder::INOUT);
  Subnet subnet = Subnet::get(builder.make());

  CutExtractor cutExtractor(&subnet, 2);
  ConeBuilder coneBuilder(&subnet);

  cutConesValid(subnet, cutExtractor, coneBuilder);
}

TEST(ConeBuilderTest, MaxCone) {
  SubnetBuilder builder;

  std::size_t inputIdx0 = builder.addCell(model::IN, SubnetBuilder::INPUT);
  std::size_t inputIdx1 = builder.addCell(model::IN, SubnetBuilder::INPUT);
  std::size_t inputIdx2 = builder.addCell(model::IN, SubnetBuilder::INPUT);
  std::size_t andIdx0 = builder.addCell(model::AND, inputIdx0, inputIdx1);
  std::size_t andIdx1 = builder.addCell(model::AND, andIdx0, inputIdx2);
  builder.addCell(model::OUT, Link(andIdx1), SubnetBuilder::OUTPUT);
  Subnet subnet = Subnet::get(builder.make());
  ConeBuilder coneBuilder(&subnet);

  maxConesValid(subnet, coneBuilder);
}

TEST(ConeBuilderTest, OverlappingLinks) {
  SubnetBuilder builder;

  std::size_t inputIdx0 = builder.addCell(model::IN, SubnetBuilder::INPUT);
  std::size_t inputIdx1 = builder.addCell(model::IN, SubnetBuilder::INPUT);
  std::size_t inputIdx2 = builder.addCell(model::IN, SubnetBuilder::INPUT);
  std::size_t andIdx0 = builder.addCell(model::AND, inputIdx0, inputIdx1);
  std::size_t andIdx1 = builder.addCell(model::AND, inputIdx1, inputIdx2);
  std::size_t andIdx2 = builder.addCell(model::AND, andIdx0, andIdx1);
  builder.addCell(model::OUT, Link(andIdx2), SubnetBuilder::OUTPUT);
  Subnet subnet = Subnet::get(builder.make());
  ConeBuilder coneBuilder(&subnet);
  maxConesValid(subnet, coneBuilder);
}

TEST(ConeBuilderTest, OverlappingLinks2) {
  SubnetBuilder builder;

  std::size_t inputIdx0 = builder.addCell(model::IN, SubnetBuilder::INPUT);
  std::size_t inputIdx1 = builder.addCell(model::IN, SubnetBuilder::INPUT);
  std::size_t andIdx0 = builder.addCell(model::AND, inputIdx1, inputIdx0);
  builder.addCell(model::AND, inputIdx1, andIdx0, SubnetBuilder::OUTPUT);
  Subnet subnet = Subnet::get(builder.make());
  ConeBuilder coneBuilder(&subnet);
  maxConesValid(subnet, coneBuilder);
}

TEST(ConeBuilderTest, OneElementMaxCone) {
  SubnetBuilder builder;

  builder.addCell(model::OUT, SubnetBuilder::INOUT);
  Subnet subnet = Subnet::get(builder.make());

  ConeBuilder coneBuilder(&subnet);

  maxConesValid(subnet, coneBuilder);
}

} // namespace eda::gate::optimizer2
