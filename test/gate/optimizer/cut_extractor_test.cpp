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

bool cutsEqual(const Cut &cut1, const Cut &cut2) {
  const auto &cut1EntryIDs = cut1.entryIDs;
  const auto &cut2EntryIDs = cut2.entryIDs;
  if (cut1.rootID != cut2.rootID ||
      cut1EntryIDs.size() != cut2EntryIDs.size()) {
    return false;
  }
  for (const auto entryID: cut1EntryIDs) {
    if (cut2EntryIDs.find(entryID) == cut2EntryIDs.end()) {
      return false;
    }
  }
  return true;
}

bool cutsSetsEqual(const CutsList &cuts1, const CutsList &cuts2) {

  if (cuts1.size() != cuts2.size()) {
    return false;
  }
  std::vector<char> cuts2IDsUsed(cuts2.size(), false);
  for (const Cut &cut1 : cuts1) {
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

bool resultValid(const CutExtractor &cutExtractor,
                 const std::vector<CutsList> &cellToCuts) {

  for (std::size_t i = 0; i < cellToCuts.size(); ++i) {
    if (!cutsSetsEqual(cutExtractor.getCuts(i), cellToCuts[i])) {

      return false;
    }
  }
  return true;
}

TEST(CutExtractorTest, OneAND) {
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(2);
  const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  builder.addOutput(andLink0);
  const Subnet &subnet = Subnet::get(builder.make());

  CutExtractor cutExtractor(&subnet, 10);
  const std::vector<CutsList> validRes {
    { Cut(0,  { 0 }) },
    { Cut(1, { 1 }) },
    { Cut(2,  { 2 }), Cut(2,  { 0, 1 }) },
    { Cut(3,  { 3 }), Cut(3, { 2 }), Cut(3,  { 1, 0 }) }
  };
  EXPECT_TRUE(resultValid(cutExtractor, validRes));
}

TEST(CutExtractorTest, TwoAND) {
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(3);
  const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  const auto andLink1 = builder.addCell(model::AND, andLink0, inputs[2]);
  builder.addOutput(andLink1);
  const Subnet &subnet = Subnet::get(builder.make());

  CutExtractor cutExtractor(&subnet, 10);
  const std::vector<CutsList> validRes {
    { Cut(0,  { 0 }) },
    { Cut(1,  { 1 }) },
    { Cut(2,  { 2 }) },
    { Cut(3, { 3 }), Cut(3,  { 0, 1 }) },
    { Cut(4,  { 4 }), Cut(4,  { 3, 2 }), Cut(4,  { 0, 1, 2 }) },
    { Cut(5,  { 5 }), Cut(5,  { 4 }), Cut(5,  { 3, 2 }),
      Cut(5, { 0, 1, 2 }) }
  };
  EXPECT_TRUE(resultValid(cutExtractor, validRes));
}

TEST(CutExtractorTest, Domination) {
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(2);
  const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  const auto bufLink0 = builder.addCell(model::BUF, andLink0);
  const auto andLink1 = builder.addCell(model::AND, andLink0, bufLink0);
  builder.addOutput(andLink1);
  const auto &subnet = Subnet::get(builder.make());

  CutExtractor cutExtractor(&subnet, 10);
  const std::vector<CutsList> validRes {
    { Cut(0,  { 0 }) },
    { Cut(1, { 1 }) },
    { Cut(2, { 2 }), Cut(2, { 1, 0 }) },
    { Cut(3, { 3 }), Cut(3, { 2 }), Cut(3, { 1, 0 }) },
    { Cut(4, { 4 }), Cut(4, { 2 }), Cut(4, { 1, 0 }) },
    { Cut(5, { 5 }), Cut(5, { 4 }), Cut(5, { 2 }),
      Cut(5, { 1, 0 }) }
  };
  EXPECT_TRUE(resultValid(cutExtractor, validRes));
}

TEST(CutExtractorTest, LimitedK) {
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

  CutExtractor cutExtractor(&subnet, 2);
  const std::vector<CutsList> validRes {
    { Cut(0,  { 0 }) },
    { Cut(1,  { 1 }), Cut(1,  { 0 }) },
    { Cut(2,  { 2 }), Cut(2,  { 0 }) },
    { Cut(3,  { 3 }), Cut(3,  { 1 }), Cut(3, { 0 }) },
    { Cut(4,  { 4 }), Cut(4, { 1 }), Cut(4,  { 0 }) },
    { Cut(5,  { 5 }), Cut(5, { 2 }), Cut(5, { 0 }) },
    { Cut(6,  { 6 }), Cut(6, { 2 }), Cut(6, { 0 }) },
    { Cut(7,  { 7 }), Cut(7,  { 3, 4 }), Cut(7, { 1 }),
      Cut(7,  { 0 }) },
    { Cut(8,  { 8 }), Cut(8,  { 5, 6 }), Cut(8, { 2 }),
      Cut(8,  { 0 }) },
    { Cut(9,  { 9 }), Cut(9, { 7, 8 }), Cut(9, { 7, 2 }),
      Cut(9,  { 8, 1 }), Cut(9, { 1, 2 }), Cut(9, { 0 }) },
    { Cut(10,  { 10 }), Cut(10,  { 9 }), Cut(10,  { 7, 8 }),
      Cut(10,  { 7, 2 }), Cut(10, { 8, 1 }), Cut(10, { 1, 2 }),
      Cut(10,  { 0 }) }
  };
  EXPECT_TRUE(resultValid(cutExtractor, validRes));
}

TEST(CutExtractorTest, CutsIntersection) {
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(3);
  const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  const auto andLink1 = builder.addCell(model::AND, inputs[1], inputs[2]);
  const auto andLink2 = builder.addCell(model::AND, andLink0, andLink1);
  builder.addOutput(andLink2);
  const auto &subnet = Subnet::get(builder.make());

  CutExtractor cutExtractor(&subnet, 3);
  const std::vector<CutsList> validRes {
    { Cut(0,  { 0 }) },
    { Cut(1,  { 1 }) },
    { Cut(2, { 2 }) },
    { Cut(3, { 3 }), Cut(3,  { 1, 0 }) },
    { Cut(4,  { 4 }), Cut(4,  { 1, 2 }) },
    { Cut(5,  { 5 }), Cut(5,  { 3, 4 }), Cut(5,  { 3, 1, 2 }),
      Cut(5,  { 4, 0, 1 }), Cut(5, { 0, 1, 2 }) },
    { Cut(6,  { 6 }), Cut(6,  { 5 }), Cut(6, { 3, 4 }),
      Cut(6,  { 3, 1, 2 }), Cut(6,  { 4, 0, 1 }), Cut(6, { 0, 1, 2 }) }
  };
  EXPECT_TRUE(resultValid(cutExtractor, validRes));
}

TEST(CutExtractorTest, NoCuts) {
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(3);
  const auto andLink0 = builder.addCell(model::AND,
                                        inputs[0], inputs[1], inputs[2]);
  builder.addOutput(andLink0);
  const auto &subnet = Subnet::get(builder.make());

  CutExtractor cutExtractor(&subnet, 2);
  const std::vector<CutsList> validRes {
    { Cut(0,  { 0 }) },
    { Cut(1, { 1 }) },
    { Cut(2,  { 2 }) },
    { Cut(3,  { 3 }) },
    { Cut(4, { 4 }), Cut(4, { 3 }) }
  };
  EXPECT_TRUE(resultValid(cutExtractor, validRes));
}

TEST(CutExtractorTest, SameElementsInCuts) {
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(1);
  const auto bufLink0 = builder.addCell(model::BUF, inputs[0]);
  const auto bufLink1 = builder.addCell(model::BUF, Link(inputs[0].idx, 0, 1));
  const auto andLink0 = builder.addCell(model::AND, bufLink0, bufLink1);
  builder.addOutput(andLink0);
  const auto &subnet = Subnet::get(builder.make());

  CutExtractor cutExtractor(&subnet, 1);
  const std::vector<CutsList> validRes {
    { Cut(0, { 0 }) },
    { Cut(1, { 1 }), Cut(1,  { 0 }) },
    { Cut(2, { 2 }), Cut(2, { 0 }) },
    { Cut(3, { 3 }), Cut(3,  { 0 }) },
    { Cut(4, { 4 }), Cut(4,  { 3 }), Cut(4,  { 0 }) }
  };
  EXPECT_TRUE(resultValid(cutExtractor, validRes));
}

TEST(CutExtractorTest, LinkEntriesInSubnet) {
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(6);
  const auto andLink0 = builder.addCell(model::AND,
                                        { inputs[0], inputs[1], inputs[2],
                                          inputs[3], inputs[4], inputs[5] });
  builder.addOutput(andLink0);
  const auto &subnet = Subnet::get(builder.make());

  CutExtractor cutExtractor(&subnet, 6);
  const std::vector<CutsList> validRes {
    { Cut(0, { 0 }) },
    { Cut(1,  { 1 }) },
    { Cut(2,  { 2 }) },
    { Cut(3,  { 3 }) },
    { Cut(4,  { 4 }) },
    { Cut(5,  { 5 }) },
    { Cut(6,  { 6 }), Cut(6,  { 0, 1, 2, 3, 4, 5 }) },
    {  },
    { Cut(8,  { 8 }), Cut(8,  { 6 }), Cut(8,  { 0, 1, 2, 3, 4, 5 }) }
  };
  EXPECT_TRUE(resultValid(cutExtractor, validRes));
}

TEST(CutExtractorTest, GetEntriesIDs) {
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(2);
  const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  builder.addOutput(andLink0);
  const auto &subnet = Subnet::get(builder.make());
  util::BoundedSet<size_t> a{2, 1};
  a.insert(0, 1);
  CutExtractor cutExtractor(&subnet, 2); 
  const std::vector<CutsEntries> validRes {
    { { 2, 0 } },
    { { 2, 1 } },
    { { 2, 2 }, a } ,
    { { 2, 3 }, { 2, 2 }, a }
  };

  for (std::size_t i = 0; i < subnet.getEntries().size(); ++i) {
    EXPECT_TRUE(cutExtractor.getCutsEntries(i) == validRes[i]);
  }
}

TEST(CutExtractorTest, LargeSubnet) {
  std::string file = "ac97_ctrl_orig";

  const auto subnetID = translator::translateGmlOpenabc(file)->make();

  const auto &subnet = Subnet::get(subnetID);
  CutExtractor cutExtractor(&subnet, 6);
}

} // namespace eda::gate::optimizer
