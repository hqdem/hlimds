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

bool coneOutputCorrect(const Subnet &coneSubnet) {
  const auto coneSubnetEntries = coneSubnet.getEntries();
  return coneSubnet.getOutNum() == 1 &&
         coneSubnetEntries[coneSubnetEntries.size() - 1].cell.isOut();
}

bool inputsAtTheBeginning(const Cone &cone) {
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
  std::size_t inputN = 0;
  const auto &subnetEntryLinks = subnet.getLinks(subnetEntryIdx);
  for (const auto coneInputLink : coneSubnet.getLinks(coneEntryIdx)) {
    const auto subnetInputLink = subnetEntryLinks[inputN];
    uint64_t subnetInputLinkIdx = subnetInputLink.idx;
    uint64_t coneInputLinkIdx = coneInputLink.idx;
    if (cone.coneEntryToOrig[coneInputLinkIdx] != subnetInputLinkIdx ||
        subnetInputLink.out != coneInputLink.out ||
        subnetInputLink.inv != coneInputLink.inv) {
      return false;
    }
    if (!coneValid(subnet, cone, coneInputLinkIdx, isMaxCone)) {
      return false;
    }
    inputN++;
  }
  return true;
}

bool cutConeValid(const Subnet &subnet,
                  const CutExtractor &cutExtractor,
                  const uint64_t subnetEntryIdx,
                  const ConeBuilder &coneBuilder) {

  const auto &cuts = cutExtractor.getCuts(subnetEntryIdx);
  for (const auto &cut : cuts) {
    const Cone cone = coneBuilder.getCone(cut);
    const Subnet &coneSubnet = Subnet::get(cone.subnetID);
    const auto &coneSubnetEntries = coneSubnet.getEntries();
    if (!coneOutputCorrect(coneSubnet) ||
        coneSubnet.getInNum() != cut.entryIdxs.size() ||
        !inputsAtTheBeginning(cone) ||
        !coneValid(subnet, cone, coneSubnetEntries.size() - 1, false)) {

      return false;
    }
  }
  return true;
}

bool maxConeValid(const Subnet &subnet,
                  const uint64_t subnetEntryIdx,
                  const ConeBuilder &coneBuilder) {

  const Cone cone = coneBuilder.getMaxCone(subnetEntryIdx);
  const Subnet &coneSubnet = Subnet::get(cone.subnetID);
  const auto &coneSubnetEntries = coneSubnet.getEntries();
  return coneOutputCorrect(coneSubnet) &&
         inputsAtTheBeginning(cone) &&
         coneValid(subnet, cone, coneSubnetEntries.size() - 1, true);
}

void conesValid(const Subnet &subnet,
                const ConeBuilder &coneBuilder,
                const CutExtractor *cutExtractor = nullptr) {

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
    if (cutExtractor) {
      EXPECT_TRUE(cutConeValid(subnet, *cutExtractor, entryIdx, coneBuilder));
    } else {
      EXPECT_TRUE(maxConeValid(subnet, entryIdx, coneBuilder));
    }
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

  conesValid(subnet, coneBuilder, &cutExtractor);
}

TEST(ConeBuilderTest, OneElementCut) {
  SubnetBuilder builder;

  builder.addOutput(builder.addInput());
  Subnet subnet = Subnet::get(builder.make());

  CutExtractor cutExtractor(&subnet, 2);
  ConeBuilder coneBuilder(&subnet);

  conesValid(subnet, coneBuilder, &cutExtractor);
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

  conesValid(subnet, coneBuilder, &cutExtractor);
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
  conesValid(subnet, coneBuilder, &cutExtractor);
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

  conesValid(subnet, coneBuilder);
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
  conesValid(subnet, coneBuilder);
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
  conesValid(subnet, coneBuilder);
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
  conesValid(subnet, coneBuilder);
}

TEST(ConeBuilderTest, OutputPort) {
  SubnetBuilder builder;

  std::size_t inputIdx0 = builder.addInput();
  std::size_t inputIdx1 = builder.addInput();
  std::size_t inputIdx2 = builder.addInput();
  std::size_t bufIdx0 = builder.addCell(model::BUF, Link(inputIdx2, 0, 1));
  std::size_t andIdx0 = builder.addCell(model::AND, bufIdx0, inputIdx1);
  std::size_t andIdx1 = builder.addCell(model::AND, bufIdx0, inputIdx0);
  std::size_t andIdx2 = builder.addCell(model::AND, bufIdx0,
                                        Link(andIdx0, 1, 0),
                                        Link(andIdx1, 1, 1));
  builder.addOutput(Link(andIdx2));
  Subnet subnet = Subnet::get(builder.make());
  ConeBuilder coneBuilder(&subnet);
  CutExtractor cutExtractor(&subnet, 10);
  conesValid(subnet, coneBuilder, &cutExtractor);
}

TEST(ConeBuilderTest, InvertorFlag) {
  SubnetBuilder builder;

  std::size_t inputIdx0 = builder.addInput();
  std::size_t inputIdx1 = builder.addInput();
  std::size_t inputIdx2 = builder.addInput();
  std::size_t bufIdx0 = builder.addCell(model::BUF, Link(inputIdx2, 0));
  std::size_t andIdx0 = builder.addCell(model::AND, bufIdx0,
                                        Link(inputIdx1, 1));
  std::size_t andIdx1 = builder.addCell(model::AND, Link(bufIdx0, 0),
                                        inputIdx0);
  std::size_t andIdx2 = builder.addCell(model::AND, bufIdx0,
                                        Link(andIdx0, 1),
                                        Link(andIdx1, 0));
  builder.addOutput(Link(andIdx2));
  Subnet subnet = Subnet::get(builder.make());
  ConeBuilder coneBuilder(&subnet);
  CutExtractor cutExtractor(&subnet, 10);
  conesValid(subnet, coneBuilder, &cutExtractor);
}

TEST(ConeBuilderTest, OneElementMaxCone) {
  SubnetBuilder builder;

  builder.addOutput(builder.addInput());
  Subnet subnet = Subnet::get(builder.make());

  ConeBuilder coneBuilder(&subnet);

  conesValid(subnet, coneBuilder);
}

} // namespace eda::gate::optimizer2
