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

bool inputsAtTheBeginning(Cone &cone) {
  bool foundNotInput = false;
  const auto &subnet = Subnet::get(cone.subnetID);
  const auto &entries = subnet.getEntries();
  for (std::size_t entryIdx = 0; entryIdx < entries.size(); ++entryIdx) {
    const auto &curCell = entries[entryIdx].cell;
    if ((curCell.isIn() || curCell.isOne() || curCell.isZero()) &&
        foundNotInput) {

      return false;
    }
    if (!(curCell.isIn() || curCell.isOne() || curCell.isZero()) &&
        !foundNotInput) {

      foundNotInput = true;
    }
    entryIdx += curCell.more;
  }
  return true;
}

bool coneValid(const Subnet &subnet,
               Cone cone,
               const uint64_t coneEntryIdx,
               const bool isMaxCone) {

  const Subnet &coneSubnet = Subnet::get(cone.subnetID);
  const auto &coneCell = coneSubnet.getEntries()[coneEntryIdx].cell;
  if (coneCell.isOut()) {
    const Link coneCellIn = coneSubnet.getLinks(coneEntryIdx)[0];
    return coneValid(subnet, cone, coneCellIn.idx, isMaxCone);
  }
  if (cone.coneEntryToOrig.find(coneEntryIdx) == cone.coneEntryToOrig.end()) {
    return false;
  }
  if (!inputsAtTheBeginning(cone)) {
    return false;
  }
  const uint64_t subnetEntryIdx = cone.coneEntryToOrig[coneEntryIdx];
  const auto &subnetCell = subnet.getEntries()[subnetEntryIdx].cell;
  if (!coneCell.isIn() && (subnetCell.getSymbol() != coneCell.getSymbol() ||
                           subnet.getLinks(subnetEntryIdx).size() !=
                           coneSubnet.getLinks(coneEntryIdx).size())) {

    return false;
  }
  if (isMaxCone && subnetCell.isIn() != coneCell.isIn()) {
    return false;
  }
  std::size_t coneInputN = 0;
  for (const auto coneInputEntry : coneSubnet.getLinks(coneEntryIdx)) {
    uint64_t coneInputEntryIdx = coneInputEntry.idx;
    const auto &subnetEntryLinks = subnet.getLinks(subnetEntryIdx);
    uint64_t subnetInputEntryIdx = subnetEntryLinks[coneInputN].idx;
    if (cone.coneEntryToOrig[coneInputEntryIdx] != subnetInputEntryIdx) {
      return false;
    }
    if (!coneValid(subnet, cone, coneInputEntryIdx, isMaxCone)) {
      return false;
    }
    coneInputN++;
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
    const auto subnetCell = subnetEntries[entryIdx].cell;
    if (subnetCell.isOut()) {
      entryIdx += subnetCell.more;
      continue;
    }
    const auto &cuts = cutExtractor.getCuts(entryIdx);
    for (const auto &cut : cuts) {
      const Cone cone = coneBuilder.getCone(cut);
      const Subnet &coneSubnet = Subnet::get(cone.subnetID);
      EXPECT_TRUE(coneSubnet.getInNum() == cut.entryIdxs.size());
      const auto &coneSubnetEntries = coneSubnet.getEntries();
      EXPECT_TRUE(coneValid(subnet, cone, coneSubnetEntries.size() - 1, false));
    }
    entryIdx += subnetCell.more;
  }
}

void maxConesValid(const Subnet &subnet,
                   const ConeBuilder &coneBuilder) {

  if (!getenv("UTOPIA_HOME")) {
    FAIL() << "UTOPIA_HOME is not set.";
  }
  const auto &subnetEntries = subnet.getEntries();
  for (uint64_t entryIdx = 0; entryIdx < subnetEntries.size(); ++entryIdx) {
    const auto subnetCell = subnetEntries[entryIdx].cell;
    if (subnetCell.isOut()) {
      entryIdx += subnetCell.more;
      continue;
    }
    const Cone cone = coneBuilder.getMaxCone(entryIdx);
    const auto &coneSubnetEntries = Subnet::get(cone.subnetID).getEntries();
    EXPECT_TRUE(coneValid(subnet, cone, coneSubnetEntries.size() - 1, true));
    entryIdx += subnetCell.more;
  }
}

TEST(ConeBuilderTest, SimpleTest) {
  SubnetBuilder builder;

  std::size_t inputIdx0 = builder.addInput();
  std::size_t inputIdx1 = builder.addInput();
  std::size_t andIdx0 = builder.addCell(model::AND, inputIdx0, inputIdx1);
  builder.addOutput(Link(andIdx0));
  Subnet subnet = Subnet::get(builder.make());

  CutExtractor cutExtractor(&subnet, 5);
  ConeBuilder coneBuilder(&subnet);

  cutConesValid(subnet, cutExtractor, coneBuilder);
}

TEST(ConeBuilderTest, OneElementCut) {
  SubnetBuilder builder;

  builder.addOutput(builder.addInput());
  Subnet subnet = Subnet::get(builder.make());

  CutExtractor cutExtractor(&subnet, 2);
  ConeBuilder coneBuilder(&subnet);

  cutConesValid(subnet, cutExtractor, coneBuilder);
}

TEST(ConeBuilderTest, CutLimit) {
  SubnetBuilder builder;

  std::size_t inputIdx0 = builder.addInput();
  std::size_t inputIdx1 = builder.addInput();
  std::size_t inputIdx2 = builder.addInput();
  std::size_t andIdx0 = builder.addCell(model::AND, inputIdx0, inputIdx1);
  std::size_t andIdx1 = builder.addCell(model::AND, andIdx0, inputIdx2);
  builder.addOutput(Link(andIdx1));
  Subnet subnet = Subnet::get(builder.make());
  CutExtractor cutExtractor(&subnet, 2);
  ConeBuilder coneBuilder(&subnet);

  cutConesValid(subnet, cutExtractor, coneBuilder);
}

TEST(ConeBuilderTest, OverlapLinks3UsagesCut) {
  SubnetBuilder builder;

  std::size_t inputIdx0 = builder.addInput();
  std::size_t inputIdx1 = builder.addInput();
  std::size_t inputIdx2 = builder.addInput();
  std::size_t bufIdx0 = builder.addCell(model::BUF, inputIdx2);
  std::size_t andIdx0 = builder.addCell(model::AND, bufIdx0, inputIdx1);
  std::size_t andIdx1 = builder.addCell(model::AND, bufIdx0, inputIdx0);
  std::size_t andIdx2 = builder.addCell(model::AND, bufIdx0, andIdx0, andIdx1);
  builder.addOutput(Link(andIdx2));
  Subnet subnet = Subnet::get(builder.make());
  CutExtractor cutExtractor(&subnet, 3);
  ConeBuilder coneBuilder(&subnet);
  cutConesValid(subnet, cutExtractor, coneBuilder);
}

TEST(ConeBuilderTest, MaxCone) {
  SubnetBuilder builder;

  std::size_t inputIdx0 = builder.addInput();
  std::size_t inputIdx1 = builder.addInput();
  std::size_t inputIdx2 = builder.addInput();
  std::size_t andIdx0 = builder.addCell(model::AND, inputIdx0, inputIdx1);
  std::size_t andIdx1 = builder.addCell(model::AND, andIdx0, inputIdx2);
  builder.addOutput(Link(andIdx1));
  Subnet subnet = Subnet::get(builder.make());
  ConeBuilder coneBuilder(&subnet);

  maxConesValid(subnet, coneBuilder);
}

TEST(ConeBuilderTest, OverlapLinks) {
  SubnetBuilder builder;

  std::size_t inputIdx0 = builder.addInput();
  std::size_t inputIdx1 = builder.addInput();
  std::size_t inputIdx2 = builder.addInput();
  std::size_t andIdx0 = builder.addCell(model::AND, inputIdx0, inputIdx1);
  std::size_t andIdx1 = builder.addCell(model::AND, inputIdx1, inputIdx2);
  std::size_t andIdx2 = builder.addCell(model::AND, andIdx0, andIdx1);
  builder.addOutput(Link(andIdx2));
  Subnet subnet = Subnet::get(builder.make());
  ConeBuilder coneBuilder(&subnet);
  maxConesValid(subnet, coneBuilder);
}

TEST(ConeBuilderTest, OverlapLinksReverse) {
  SubnetBuilder builder;

  std::size_t inputIdx0 = builder.addInput();
  std::size_t inputIdx1 = builder.addInput();
  std::size_t andIdx0 = builder.addCell(model::AND, inputIdx1, inputIdx0);
  std::size_t andIdx1 = builder.addCell(model::AND, inputIdx1, andIdx0);
  builder.addOutput(Link(andIdx1));
  Subnet subnet = Subnet::get(builder.make());
  ConeBuilder coneBuilder(&subnet);
  maxConesValid(subnet, coneBuilder);
}

TEST(ConeBuilderTest, OverlapLinks3UsagesMax) {
  SubnetBuilder builder;

  std::size_t inputIdx0 = builder.addInput();
  std::size_t inputIdx1 = builder.addInput();
  std::size_t inputIdx2 = builder.addInput();
  std::size_t bufIdx0 = builder.addCell(model::BUF, inputIdx2);
  std::size_t andIdx0 = builder.addCell(model::AND, bufIdx0, inputIdx1);
  std::size_t andIdx1 = builder.addCell(model::AND, bufIdx0, inputIdx0);
  std::size_t andIdx2 = builder.addCell(model::AND, bufIdx0, andIdx0, andIdx1);
  builder.addOutput(Link(andIdx2));
  Subnet subnet = Subnet::get(builder.make());
  ConeBuilder coneBuilder(&subnet);
  maxConesValid(subnet, coneBuilder);
}

TEST(ConeBuilderTest, OneElementMaxCone) {
  SubnetBuilder builder;

  builder.addOutput(builder.addInput());
  Subnet subnet = Subnet::get(builder.make());

  ConeBuilder coneBuilder(&subnet);

  maxConesValid(subnet, coneBuilder);
}

} // namespace eda::gate::optimizer2
