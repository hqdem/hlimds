//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/subnetview.h"

#include "gtest/gtest.h"

namespace eda::gate::optimizer {

using Subnet = model::Subnet;
using SubnetID = model::SubnetID;
using SubnetBuilder = model::SubnetBuilder;
using SubnetView = model::SubnetView;
using Link = model::Subnet::Link;
using Cut = CutExtractor::Cut;

static size_t getConeRootIdx(const Subnet &coneSubnet) {
  return coneSubnet.getLink(coneSubnet.getMaxIdx(), 0).idx;
}

static bool coneOutputCorrect(const Subnet &coneSubnet) {
  const auto &coneSubnetEntries = coneSubnet.getEntries();
  return coneSubnet.getOutNum() == 1 &&
         coneSubnetEntries[coneSubnetEntries.size() - 1].cell.isOut();
}

static bool inputsAtTheBeginning(const Subnet &coneSubnet) {
  bool foundNotInput = false;
  const auto &entries = coneSubnet.getEntries();
  for (size_t entryIdx = 0; entryIdx < entries.size(); ++entryIdx) {
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

static bool coneValid(const SubnetBuilder &builder,
                      const Subnet &coneSubnet,
                      const size_t origEntryIdx,
                      const size_t coneEntryIdx,
                      const bool isMaxCone) {
  const auto &coneEntries = coneSubnet.getEntries();
  const auto &coneCell = coneEntries[coneEntryIdx].cell;
  const auto &origCell = builder.getCell(origEntryIdx);

  const auto coneSymbol = coneCell.getSymbol();
  const auto origSymbol = origCell.getSymbol();

  const auto coneLinks = coneSubnet.getLinks(coneEntryIdx);
  const auto origLinks = builder.getLinks(origEntryIdx);

  if (!coneCell.isIn() && origSymbol != coneSymbol) {
    return false;
  }

  if (!coneCell.isIn() && origLinks.size() != coneLinks.size()) {
    return false;
  }

  if (isMaxCone && origCell.isIn() != coneCell.isIn()) {
    return false;
  }

  size_t inputN = 0;
  for (const auto coneLink : coneLinks) {
    const auto origLink = origLinks[inputN];

    const size_t origLinkIdx = origLink.idx;
    const size_t coneLinkIdx = coneLink.idx;

    if (origLink.out != coneLink.out ||
        origLink.inv != coneLink.inv) {
      return false;
    }
    if (!coneValid(builder, coneSubnet, origLinkIdx, coneLinkIdx, isMaxCone)) {
      return false;
    }
    inputN++;
  }

  return true;
}

static bool cutConeValid(const SubnetBuilder &builder,
                         const CutExtractor &cutExtractor,
                         const size_t origEntryIdx) {
  const auto &cuts = cutExtractor.getCuts(origEntryIdx);
  for (const auto &cut : cuts) {
    SubnetView cone(builder, cut);
    const auto &coneSubnet = cone.getSubnet().makeObject();
    const auto coneEntryIdx = getConeRootIdx(coneSubnet);

    if (!coneOutputCorrect(coneSubnet) ||
        coneSubnet.getInNum() != cut.entryIdxs.size() ||
        !inputsAtTheBeginning(coneSubnet) ||
        !coneValid(builder, coneSubnet, origEntryIdx, coneEntryIdx, false)) {
      return false;
    }
  }
  return true;
}

static bool maxConeValid(const SubnetBuilder &builder,
                         const size_t origEntryIdx) {
  SubnetView cone(builder, origEntryIdx);
  const auto &coneSubnet = cone.getSubnet().makeObject();
  const auto coneEntryIdx = getConeRootIdx(coneSubnet);

  return coneOutputCorrect(coneSubnet) &&
         inputsAtTheBeginning(coneSubnet) &&
         coneValid(builder, coneSubnet, origEntryIdx, coneEntryIdx, true);
}

static void conesValid(const SubnetBuilder &builder,
                       const CutExtractor *cutExtractor = nullptr) {
  for (auto it = builder.begin(); it != builder.end(); it.nextCell()) {
    if (builder.getCell(*it).isOut()) {
      continue;
    }
    EXPECT_TRUE(cutExtractor ?
                cutConeValid(builder, *cutExtractor, *it) :
                maxConeValid(builder, *it));
  }
}

TEST(ConeBuilderTest, SimpleTest) {
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(2);
  const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  builder.addOutput(andLink0);

  CutExtractor cutExtractor(&builder, 5, true);
  conesValid(builder, &cutExtractor);
}

TEST(ConeBuilderTest, OneElementCut) {
  SubnetBuilder builder;

  builder.addOutput(builder.addInput());

  CutExtractor cutExtractor(&builder, 2, true);
  conesValid(builder, &cutExtractor);
}

TEST(ConeBuilderTest, CutLimit) {
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(3);
  const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  const auto andLink1 = builder.addCell(model::AND, andLink0, inputs[2]);
  builder.addOutput(andLink1);

  CutExtractor cutExtractor(&builder, 2, true);
  conesValid(builder, &cutExtractor);
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

  CutExtractor cutExtractor(&builder, 3, true);
  conesValid(builder, &cutExtractor);
}

TEST(ConeBuilderTest, MaxCone) {
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(3);
  const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  const auto andLink1 = builder.addCell(model::AND, andLink0, inputs[2]);
  builder.addOutput(andLink1);

  conesValid(builder);
}

TEST(ConeBuilderTest, OverlapLinks) {
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(3);
  const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  const auto andLink1 = builder.addCell(model::AND, inputs[1], inputs[2]);
  const auto andLink2 = builder.addCell(model::AND, andLink0, andLink1);
  builder.addOutput(andLink2);

  conesValid(builder);
}

TEST(ConeBuilderTest, OverlapLinksReverse) {
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(2);
  const auto andLink0 = builder.addCell(model::AND, inputs[1], inputs[0]);
  const auto andLink1 = builder.addCell(model::AND, inputs[1], andLink0);
  builder.addOutput(andLink1);

  conesValid(builder);
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

  conesValid(builder);
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

  CutExtractor cutExtractor(&builder, 10, true);
  conesValid(builder, &cutExtractor);
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

  CutExtractor cutExtractor(&builder, 10, true);
  conesValid(builder, &cutExtractor);
}

TEST(ConeBuilderTest, OneElementMaxCone) {
  SubnetBuilder builder;

  builder.addOutput(builder.addInput());

  conesValid(builder);
}

} // namespace eda::gate::optimizer
