//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/utils/subnet_random.h"
#include "gate/optimizer/cut_extractor.h"
#include "gate/translator/graphml_test_utils.h"
#include "util/env.h"

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace eda::gate::optimizer {

using Subnet = model::Subnet;
using SubnetID = model::SubnetID;
using SubnetBuilder = model::SubnetBuilder;
using Link = model::Subnet::Link;
using CutsEntries = CutExtractor::CutsEntries;

static bool cutsEqual(const Cut &cut1, const Cut &cut2) {
  const auto &cut1LeafIDs = cut1.leafIDs;
  const auto &cut2LeafIDs = cut2.leafIDs;
  if (cut1.rootID != cut2.rootID ||
      cut1LeafIDs.size() != cut2LeafIDs.size()) {
    return false;
  }
  for (const auto leafID: cut1LeafIDs) {
    if (cut2LeafIDs.find(leafID) == cut2LeafIDs.end()) {
      return false;
    }
  }
  return true;
}

static bool cutsSetsEqual(const CutsList &cuts1, const CutsList &cuts2) {
  if (cuts1.size() != cuts2.size()) {
    return false;
  }
  std::vector<char> cuts2IDsUsed(cuts2.size(), false);
  for (const auto &cut1 : cuts1) {
    bool foundEqualCut = false;
    for (std::size_t i = 0; i < cuts2.size(); ++i) {
      if (!cuts2IDsUsed[i] && cutsEqual(cut1, cuts2[i])) {
        cuts2IDsUsed[i] = true;
        foundEqualCut = true;
      }
    }
    if (!foundEqualCut) {
      return false;
    }
  }
  return true;
}

static bool resultValid(const CutExtractor &cutExtractor,
                        const std::vector<CutsList> &cellToCuts) {
  for (std::size_t i = 0; i < cellToCuts.size(); ++i) {
    if (!cutsSetsEqual(cutExtractor.getCuts(i), cellToCuts[i])) {
      return false;
    }
  }
  return true;
}

TEST(CutExtractorTest, OneAND) {
  constexpr auto k = 10;
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(2);
  const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  builder.addOutput(andLink0);
  const auto &subnet = Subnet::get(builder.make());

  CutExtractor cutExtractor(&subnet, k);
  const std::vector<CutsList> validRes {
    { Cut(k, 0, { 0 }, true) },
    { Cut(k, 1, { 1 }, true) },
    { Cut(k, 2, { 2 }, true), Cut(k, 2, { 0, 1 }, true) },
    { Cut(k, 3, { 3 }, true), Cut(k, 3, { 2 }, true),
      Cut(k, 3, { 1, 0 }, true) }
  };
  EXPECT_TRUE(resultValid(cutExtractor, validRes));
}

TEST(CutExtractorTest, TwoAND) {
  constexpr auto k = 10;
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(3);
  const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  const auto andLink1 = builder.addCell(model::AND, andLink0, inputs[2]);
  builder.addOutput(andLink1);
  const auto &subnet = Subnet::get(builder.make());

  CutExtractor cutExtractor(&subnet, k);
  const std::vector<CutsList> validRes {
    { Cut(k, 0, { 0 }, true) },
    { Cut(k, 1, { 1 }, true) },
    { Cut(k, 2, { 2 }, true) },
    { Cut(k, 3, { 3 }, true), Cut(k, 3, { 0, 1 }, true) },
    { Cut(k, 4, { 4 }, true), Cut(k, 4, { 3, 2 }, true),
      Cut(k, 4, { 0, 1, 2 }, true) },
    { Cut(k, 5, { 5 }, true), Cut(k, 5, { 4 }, true), Cut(k, 5, { 3, 2 }, true),
      Cut(k, 5, { 0, 1, 2 }, true) }
  };
  EXPECT_TRUE(resultValid(cutExtractor, validRes));
}

TEST(CutExtractorTest, Domination) {
  constexpr auto k = 10;
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(2);
  const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  const auto bufLink0 = builder.addCell(model::BUF, andLink0);
  const auto andLink1 = builder.addCell(model::AND, andLink0, bufLink0);
  builder.addOutput(andLink1);
  const auto &subnet = Subnet::get(builder.make());

  CutExtractor cutExtractor(&subnet, k);
  const std::vector<CutsList> validRes {
    { Cut(k, 0, { 0 }, true) },
    { Cut(k, 1, { 1 }, true) },
    { Cut(k, 2, { 2 }, true), Cut(k, 2, { 1, 0 }, true) },
    { Cut(k, 3, { 3 }, true), Cut(k, 3, { 2 }, true),
      Cut(k, 3, { 1, 0 }, true) },
    { Cut(k, 4, { 4 }, true), Cut(k, 4, { 2 }, true),
      Cut(k, 4, { 1, 0 }, true) },
    { Cut(k, 5, { 5 }, true), Cut(k, 5, { 4 }, true), Cut(k, 5, { 2 }, true),
      Cut(k, 5, { 1, 0 }, true) }
  };
  EXPECT_TRUE(resultValid(cutExtractor, validRes));
}

TEST(CutExtractorTest, LimitedK) {
  constexpr auto k = 2;
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(1);
  const auto bufLink0 = builder.addCell(model::BUF, inputs[0]);
  const auto bufLink1 = builder.addCell(model::BUF, Link(inputs[0].idx, 0, 1));
  const auto bufLink2 = builder.addCell(model::BUF, bufLink0);
  const auto bufLink3 = builder.addCell(model::BUF, Link(bufLink0.idx, 0, 1));
  const auto bufLink4 = builder.addCell(model::BUF, bufLink1);
  const auto bufLink5 = builder.addCell(model::BUF, Link(bufLink1.idx, 0, 1));
  const auto andLink0 = builder.addCell(model::AND, bufLink2, bufLink3);
  const auto andLink1 = builder.addCell(model::AND, bufLink4, bufLink5);
  const auto andLink2 = builder.addCell(model::AND, andLink0, andLink1);
  builder.addOutput(andLink2);
  const auto &subnet = Subnet::get(builder.make());

  CutExtractor cutExtractor(&subnet, k);
  const std::vector<CutsList> validRes {
    { Cut(k, 0, { 0 }, true) },
    { Cut(k, 1, { 1 }, true), Cut(k, 1, { 0 }, true) },
    { Cut(k, 2, { 2 }, true), Cut(k, 2, { 0 }, true) },
    { Cut(k, 3, { 3 }, true), Cut(k, 3, { 1 }, true), Cut(k, 3, { 0 }, true) },
    { Cut(k, 4, { 4 }, true), Cut(k, 4, { 1 }, true), Cut(k, 4, { 0 }, true) },
    { Cut(k, 5, { 5 }, true), Cut(k, 5, { 2 }, true), Cut(k, 5, { 0 }, true) },
    { Cut(k, 6, { 6 }, true), Cut(k, 6, { 2 }, true), Cut(k, 6, { 0 }, true) },
    { Cut(k, 7, { 7 }, true), Cut(k, 7, { 3, 4 }, true), Cut(k, 7, { 1 }, true),
      Cut(k, 7, { 0 }, true) },
    { Cut(k, 8, { 8 }, true), Cut(k, 8, { 5, 6 }, true), Cut(k, 8, { 2 }, true),
      Cut(k, 8, { 0 }, true) },
    { Cut(k, 9, { 9 }, true), Cut(k, 9, { 7, 8 }, true), Cut(k, 9, { 7, 2 }, true),
      Cut(k, 9, { 8, 1 }, true), Cut(k, 9, { 1, 2 }, true),
      Cut(k, 9, { 0 }, true) },
    { Cut(k, 10, { 10 }, true), Cut(k, 10,  { 9 }, true),
      Cut(k, 10,  { 7, 8 }, true),
      Cut(k, 10, { 7, 2 }, true), Cut(k, 10, { 8, 1 }, true), Cut(k, 10, { 1, 2 }, true),
      Cut(k, 10, { 0 }, true) }
  };
  EXPECT_TRUE(resultValid(cutExtractor, validRes));
}

TEST(CutExtractorTest, CutsIntersection) {
  constexpr auto k = 3;
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(3);
  const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  const auto andLink1 = builder.addCell(model::AND, inputs[1], inputs[2]);
  const auto andLink2 = builder.addCell(model::AND, andLink0, andLink1);
  builder.addOutput(andLink2);
  const auto &subnet = Subnet::get(builder.make());

  CutExtractor cutExtractor(&subnet, k);
  const std::vector<CutsList> validRes {
    { Cut(k, 0, { 0 }, true) },
    { Cut(k, 1, { 1 }, true) },
    { Cut(k, 2, { 2 }, true) },
    { Cut(k, 3, { 3 }, true), Cut(k, 3, { 1, 0 }, true) },
    { Cut(k, 4, { 4 }, true), Cut(k, 4, { 1, 2 }, true) },
    { Cut(k, 5, { 5 }, true), Cut(k, 5, { 3, 4 }, true),
      Cut(k, 5, { 3, 1, 2 }, true),
      Cut(k, 5, { 4, 0, 1 }, true), Cut(k, 5, { 0, 1, 2 }, true) },
    { Cut(k, 6, { 6 }, true), Cut(k, 6, { 5 }, true), Cut(k, 6, { 3, 4 }, true),
      Cut(k, 6, { 3, 1, 2 }, true), Cut(k, 6, { 4, 0, 1 }, true),
      Cut(k, 6, { 0, 1, 2 }, true) }
  };
  EXPECT_TRUE(resultValid(cutExtractor, validRes));
}

TEST(CutExtractorTest, NoCuts) {
  constexpr auto k = 2;
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(3);
  const auto andLink0 = builder.addCell(model::AND,
                                        inputs[0], inputs[1], inputs[2]);
  builder.addOutput(andLink0);
  const auto &subnet = Subnet::get(builder.make());

  CutExtractor cutExtractor(&subnet, k);
  const std::vector<CutsList> validRes {
    { Cut(k, 0, { 0 }, true) },
    { Cut(k, 1, { 1 }, true) },
    { Cut(k, 2, { 2 }, true) },
    { Cut(k, 3, { 3 }, true) },
    { Cut(k, 4, { 4 }, true), Cut(k, 4, { 3 }, true) }
  };
  EXPECT_TRUE(resultValid(cutExtractor, validRes));
}

TEST(CutExtractorTest, SameElementsInCuts) {
  constexpr auto k = 1;
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(1);
  const auto bufLink0 = builder.addCell(model::BUF, inputs[0]);
  const auto bufLink1 = builder.addCell(model::BUF, Link(inputs[0].idx, 0, 1));
  const auto andLink0 = builder.addCell(model::AND, bufLink0, bufLink1);
  builder.addOutput(andLink0);
  const auto &subnet = Subnet::get(builder.make());

  CutExtractor cutExtractor(&subnet, k);
  const std::vector<CutsList> validRes {
    { Cut(k, 0, { 0 }, true) },
    { Cut(k, 1, { 1 }, true), Cut(k, 1, { 0 }, true) },
    { Cut(k, 2, { 2 }, true), Cut(k, 2, { 0 }, true) },
    { Cut(k, 3, { 3 }, true), Cut(k, 3, { 0 }, true) },
    { Cut(k, 4, { 4 }, true), Cut(k, 4, { 3 }, true), Cut(k, 4, { 0 }, true) }
  };
  EXPECT_TRUE(resultValid(cutExtractor, validRes));
}

TEST(CutExtractorTest, LinkEntriesInSubnet) {
  constexpr auto k = 6;
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(6);
  const auto andLink0 = builder.addCell(model::AND,
                                        { inputs[0], inputs[1], inputs[2],
                                          inputs[3], inputs[4], inputs[5] });
  builder.addOutput(andLink0);
  const auto &subnet = Subnet::get(builder.make());

  CutExtractor cutExtractor(&subnet, k);
  const std::vector<CutsList> validRes {
    { Cut(k, 0, { 0 }, true) },
    { Cut(k, 1, { 1 }, true) },
    { Cut(k, 2, { 2 }, true) },
    { Cut(k, 3, { 3 }, true) },
    { Cut(k, 4, { 4 }, true) },
    { Cut(k, 5, { 5 }, true) },
    { Cut(k, 6, { 6 }, true), Cut(k, 6, { 0, 1, 2, 3, 4, 5 }, true) },
    {  },
    { Cut(k, 8, { 8 }, true), Cut(k, 8, { 6 }, true),
      Cut(k, 8, { 0, 1, 2, 3, 4, 5 }, true) }
  };
  EXPECT_TRUE(resultValid(cutExtractor, validRes));
}

TEST(CutExtractorTest, GetEntriesIDs) {
  const auto k = 2;
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(2);
  const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  builder.addOutput(andLink0);
  const auto &subnet = Subnet::get(builder.make());
  Cut::Set a(k, { 0, 1 }, true);
  CutExtractor cutExtractor(&subnet, k); 
  const std::vector<CutsEntries> validRes {
    { Cut::Set(k, 0, true) },
    { Cut::Set(k, 1, true) },
    { Cut::Set(k, 2, true), a } ,
    { Cut::Set(k, 3, true), Cut::Set(k, 2, true), a }
  };

  for (std::size_t i = 0; i < subnet.getEntries().size(); ++i) {
    EXPECT_TRUE(cutExtractor.getCutsEntries(i) == validRes[i]);
  }
}

TEST(CutExtractorTest, LargeSubnet) {
  const auto k = 6;
  const std::string file = "ac97_ctrl_orig";

  const auto subnetID = translator::translateGmlOpenabc(file)->make();

  const auto &subnet = Subnet::get(subnetID);
  CutExtractor cutExtractor(&subnet, k);
}

} // namespace eda::gate::optimizer
