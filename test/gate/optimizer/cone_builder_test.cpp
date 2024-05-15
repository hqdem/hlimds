//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/cone_builder.h"

#include "gtest/gtest.h"

namespace eda::gate::optimizer {

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
               size_t coneEntryIdx,
               const bool isMaxCone) {

  const Subnet &coneSubnet = Subnet::get(cone.subnetID);
  const auto &coneEntries = coneSubnet.getEntries();
  if (cone.coneEntryToOrig.size() <= coneEntryIdx) {
    return false;
  }
  const size_t subnetEntryIdx = cone.coneEntryToOrig[coneEntryIdx];
  if (coneEntryIdx == coneEntries.size() - 1) {
    coneEntryIdx = coneSubnet.getOut(0).idx;
  }
  const auto &coneCell = coneEntries[coneEntryIdx].cell;
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
    size_t subnetInputLinkIdx = subnetInputLink.idx;
    size_t coneInputLinkIdx = coneInputLink.idx;
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
                  const size_t subnetEntryIdx,
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
                  const size_t subnetEntryIdx,
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
  for (size_t entryIdx = 0; entryIdx < subnetEntries.size(); ++entryIdx) {
    const auto subnetCell = subnetEntries[entryIdx].cell;
    if (subnetCell.isOut()) {
      entryIdx += subnetCell.more;
      continue;
    }
    EXPECT_TRUE(cutExtractor ?
                cutConeValid(subnet, *cutExtractor, entryIdx, coneBuilder) :
                maxConeValid(subnet, entryIdx, coneBuilder));
    entryIdx += subnetCell.more;
  }
}

TEST(ConeBuilderTest, SimpleTest) {
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(2);
  const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  builder.addOutput(andLink0);

  const auto &subnet = Subnet::get(builder.make());
  CutExtractor cutExtractor(&subnet, 5);
  ConeBuilder coneBuilder(&subnet);
  conesValid(subnet, coneBuilder, &cutExtractor);
}

TEST(ConeBuilderTest, OneElementCut) {
  SubnetBuilder builder;

  builder.addOutput(builder.addInput());

  const auto &subnet = Subnet::get(builder.make());
  CutExtractor cutExtractor(&subnet, 2);
  ConeBuilder coneBuilder(&subnet);
  conesValid(subnet, coneBuilder, &cutExtractor);
}

TEST(ConeBuilderTest, CutLimit) {
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(3);
  const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  const auto andLink1 = builder.addCell(model::AND, andLink0, inputs[2]);
  builder.addOutput(andLink1);

  const auto &subnet = Subnet::get(builder.make());
  CutExtractor cutExtractor(&subnet, 2);
  ConeBuilder coneBuilder(&subnet);
  conesValid(subnet, coneBuilder, &cutExtractor);
}

TEST(ConeBuilderTest, OverlapLinks3UsagesCut) {
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(3);
  const auto bufLink0 = builder.addCell(model::BUF, inputs[2]);
  const auto andLink0 = builder.addCell(model::AND, bufLink0, inputs[1]);
  const auto andLink1 = builder.addCell(model::AND, bufLink0, inputs[0]);
  const auto andLink2 = builder.addCell(model::AND, bufLink0, andLink0,
                                        andLink1);
  builder.addOutput(andLink2);

  const auto &subnet = Subnet::get(builder.make());
  CutExtractor cutExtractor(&subnet, 3);
  ConeBuilder coneBuilder(&subnet);
  conesValid(subnet, coneBuilder, &cutExtractor);
}

TEST(ConeBuilderTest, MaxCone) {
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(3);
  const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  const auto andLink1 = builder.addCell(model::AND, andLink0, inputs[2]);
  builder.addOutput(andLink1);

  const auto &subnet = Subnet::get(builder.make());
  ConeBuilder coneBuilder(&subnet);
  conesValid(subnet, coneBuilder);
}

TEST(ConeBuilderTest, OverlapLinks) {
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(3);
  const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  const auto andLink1 = builder.addCell(model::AND, inputs[1], inputs[2]);
  const auto andLink2 = builder.addCell(model::AND, andLink0, andLink1);
  builder.addOutput(andLink2);

  const auto &subnet = Subnet::get(builder.make());
  ConeBuilder coneBuilder(&subnet);
  conesValid(subnet, coneBuilder);
}

TEST(ConeBuilderTest, OverlapLinksReverse) {
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(2);
  const auto andLink0 = builder.addCell(model::AND, inputs[1], inputs[0]);
  const auto andLink1 = builder.addCell(model::AND, inputs[1], andLink0);
  builder.addOutput(andLink1);

  const auto &subnet = Subnet::get(builder.make());
  ConeBuilder coneBuilder(&subnet);
  conesValid(subnet, coneBuilder);
}

TEST(ConeBuilderTest, OverlapLinks3UsagesMax) {
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(3);
  const auto bufLink0 = builder.addCell(model::BUF, inputs[2]);
  const auto andLink0 = builder.addCell(model::AND, bufLink0, inputs[1]);
  const auto andLink1 = builder.addCell(model::AND, bufLink0, inputs[0]);
  const auto andLink2 = builder.addCell(model::AND, bufLink0, andLink0,
                                        andLink1);
  builder.addOutput(andLink2);

  const auto &subnet = Subnet::get(builder.make());
  ConeBuilder coneBuilder(&subnet);
  conesValid(subnet, coneBuilder);
}

TEST(ConeBuilderTest, OutputPort) {
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(3);
  const auto bufLink0 = builder.addCell(model::BUF, ~inputs[2]);
  const auto andLink0 = builder.addCell(model::AND, bufLink0, inputs[1]);
  const auto andLink1 = builder.addCell(model::AND, bufLink0, inputs[0]);
  const auto andLink2 = builder.addCell(model::AND, bufLink0, andLink0,
                                        ~andLink1);
  builder.addOutput(andLink2);

  const auto &subnet = Subnet::get(builder.make());
  ConeBuilder coneBuilder(&subnet);
  CutExtractor cutExtractor(&subnet, 10);
  conesValid(subnet, coneBuilder, &cutExtractor);
}

TEST(ConeBuilderTest, InvertorFlag) {
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(3);
  const auto bufLink0 = builder.addCell(model::BUF, inputs[2]);
  const auto andLink0 = builder.addCell(model::AND, bufLink0, ~inputs[1]);
  const auto andLink1 = builder.addCell(model::AND, bufLink0, inputs[0]);
  const auto andLink2 = builder.addCell(model::AND, bufLink0, ~andLink0,
                                        andLink1);
  builder.addOutput(andLink2);

  const auto &subnet = Subnet::get(builder.make());
  ConeBuilder coneBuilder(&subnet);
  CutExtractor cutExtractor(&subnet, 10);
  conesValid(subnet, coneBuilder, &cutExtractor);
}

TEST(ConeBuilderTest, OneElementMaxCone) {
  SubnetBuilder builder;

  builder.addOutput(builder.addInput());

  const auto &subnet = Subnet::get(builder.make());
  ConeBuilder coneBuilder(&subnet);
  conesValid(subnet, coneBuilder);
}

} // namespace eda::gate::optimizer
