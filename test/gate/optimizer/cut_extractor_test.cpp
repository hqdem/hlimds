//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/utils/subnet_random.h"
#include "gate/optimizer/cut_extractor.h"
#include "gate/parser/graphml_parser.h"
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
using Cut = optimizer::CutExtractor::Cut;
using CutsEntries = CutExtractor::CutsEntries;
using CutsList = CutExtractor::CutsList;
using GraphMlParser = eda::gate::parser::graphml::GraphMlParser;
using ParserData = GraphMlParser::ParserData;

bool cutsEqual(const Cut &cut1, const Cut &cut2) {
  const auto &cut1EntryIdxs = cut1.entryIdxs;
  const auto &cut2EntryIdxs = cut2.entryIdxs;
  if (cut1.rootEntryIdx != cut2.rootEntryIdx ||
      cut1.signature != cut2.signature ||
      cut1EntryIdxs.size() != cut2EntryIdxs.size()) {
    return false;
  }
  for (const auto entryIdx: cut1EntryIdxs) {
    if (cut2EntryIdxs.find(entryIdx) == cut2EntryIdxs.end()) {
      return false;
    }
  }
  return true;
}

bool cutsSetsEqual(const CutsList &cuts1, const CutsList &cuts2) {

  if (cuts1.size() != cuts2.size()) {
    return false;
  }
  std::vector<char> cuts2IdxsUsed(cuts2.size(), false);
  for (const Cut &cut1 : cuts1) {
    bool foundEqualCut = false;
    for (std::size_t i = 0; i < cuts2.size(); ++i) {
      if (!cuts2IdxsUsed[i] && cutsEqual(cut1, cuts2[i])) {
        cuts2IdxsUsed[i] = true;
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
    { Cut(0, 1, { 0 }) },
    { Cut(1, 2, { 1 }) },
    { Cut(2, 4, { 2 }), Cut(2, 3, { 0, 1 }) },
    { Cut(3, 8, { 3 }), Cut(3, 4, { 2 }), Cut(3, 3, { 1, 0 }) }
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
    { Cut(0, 1, { 0 }) },
    { Cut(1, 2, { 1 }) },
    { Cut(2, 4, { 2 }) },
    { Cut(3, 8, { 3 }), Cut(3, 3, { 0, 1 }) },
    { Cut(4, 16, { 4 }), Cut(4, 12, { 3, 2 }), Cut(4, 7, { 0, 1, 2 }) },
    { Cut(5, 32, { 5 }), Cut(5, 16, { 4 }), Cut(5, 12, { 3, 2 }),
      Cut(5, 7, { 0, 1, 2 }) }
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
    { Cut(0, 1, { 0 }) },
    { Cut(1, 2, { 1 }) },
    { Cut(2, 4, { 2 }), Cut(2, 3, { 1, 0 }) },
    { Cut(3, 8, { 3 }), Cut(3, 4, { 2 }), Cut(3, 3, { 1, 0 }) },
    { Cut(4, 16, { 4 }), Cut(4, 4, { 2 }), Cut(4, 3, { 1, 0 }) },
    { Cut(5, 32, { 5 }), Cut(5, 16, { 4 }), Cut(5, 4, { 2 }),
      Cut(5, 3, { 1, 0 }) }
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
    { Cut(0, 1, { 0 }) },
    { Cut(1, 2, { 1 }), Cut(1, 1, { 0 }) },
    { Cut(2, 4, { 2 }), Cut(2, 1, { 0 }) },
    { Cut(3, 8, { 3 }), Cut(3, 2, { 1 }), Cut(3, 1, { 0 }) },
    { Cut(4, 16, { 4 }), Cut(4, 2, { 1 }), Cut(4, 1, { 0 }) },
    { Cut(5, 32, { 5 }), Cut(5, 4, { 2 }), Cut(5, 1, { 0 }) },
    { Cut(6, 64, { 6 }), Cut(6, 4, { 2 }), Cut(6, 1, { 0 }) },
    { Cut(7, 128, { 7 }), Cut(7, 24, { 3, 4 }), Cut(7, 2, { 1 }),
      Cut(7, 1, { 0 }) },
    { Cut(8, 256, { 8 }), Cut(8, 96, { 5, 6 }), Cut(8, 4, { 2 }),
      Cut(8, 1, { 0 }) },
    { Cut(9, 512, { 9 }), Cut(9, 384, { 7, 8 }), Cut(9, 132, { 7, 2 }),
      Cut(9, 258, { 8, 1 }), Cut(9, 6, { 1, 2 }), Cut(9, 1, { 0 }) },
    { Cut(10, 1024, { 10 }), Cut(10, 512, { 9 }), Cut(10, 384, { 7, 8 }),
      Cut(10, 132, { 7, 2 }), Cut(10, 258, { 8, 1 }), Cut(10, 6, { 1, 2 }),
      Cut(10, 1, { 0 }) }
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
    { Cut(0, 1, { 0 }) },
    { Cut(1, 2, { 1 }) },
    { Cut(2, 4, { 2 }) },
    { Cut(3, 8, { 3 }), Cut(3, 3, { 1, 0 }) },
    { Cut(4, 16, { 4 }), Cut(4, 6, { 1, 2 }) },
    { Cut(5, 32, { 5 }), Cut(5, 24, { 3, 4 }), Cut(5, 14, { 3, 1, 2 }),
      Cut(5, 19, { 4, 0, 1 }), Cut(5, 7, { 0, 1, 2 }) },
    { Cut(6, 64, { 6 }), Cut(6, 32, { 5 }), Cut(6, 24, { 3, 4 }),
      Cut(6, 14, { 3, 1, 2 }), Cut(6, 19, { 4, 0, 1 }), Cut(6, 7, { 0, 1, 2 }) }
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
    { Cut(0, 1, { 0 }) },
    { Cut(1, 2, { 1 }) },
    { Cut(2, 4, { 2 }) },
    { Cut(3, 8, { 3 }) },
    { Cut(4, 16, { 4 }), Cut(4, 8, { 3 }) }
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
    { Cut(0, 1, { 0 }) },
    { Cut(1, 2, { 1 }), Cut(1, 1, { 0 }) },
    { Cut(2, 4, { 2 }), Cut(2, 1, { 0 }) },
    { Cut(3, 8, { 3 }), Cut(3, 1, { 0 }) },
    { Cut(4, 16, { 4 }), Cut(4, 8, { 3 }), Cut(4, 1, { 0 }) }
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
    { Cut(0, 1, { 0 }) },
    { Cut(1, 2, { 1 }) },
    { Cut(2, 4, { 2 }) },
    { Cut(3, 8, { 3 }) },
    { Cut(4, 16, { 4 }) },
    { Cut(5, 32, { 5 }) },
    { Cut(6, 64, { 6 }), Cut(6, 63, { 0, 1, 2, 3, 4, 5 }) },
    {  },
    { Cut(8, 256, { 8 }), Cut(8, 64, { 6 }), Cut(8, 63, { 0, 1, 2, 3, 4, 5 }) }
  };
  EXPECT_TRUE(resultValid(cutExtractor, validRes));
}

TEST(CutExtractorTest, GetEntriesIdxs) {
  SubnetBuilder builder;

  const auto inputs = builder.addInputs(2);
  const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  builder.addOutput(andLink0);
  const auto &subnet = Subnet::get(builder.make());

  CutExtractor cutExtractor(&subnet, 2);
  const std::vector<CutsEntries> validRes {
    { { 0 } },
    { { 1 } },
    { { 2 }, { 0, 1 } },
    { { 3 }, { 2 }, { 0, 1 } }
  };

  for (std::size_t i = 0; i < subnet.getEntries().size(); ++i) {
    EXPECT_TRUE(validRes[i] == cutExtractor.getCutsEntries(i));
  }
}

TEST(CutExtractorTest, LargeSubnet) {
  using path = std::filesystem::path;

  std::string filename = "ac97_ctrl_orig.bench.graphml";

  const path dir = path("test") / "data" / "gate" / "parser" / "graphml" /
      "OpenABC" / "graphml_openabcd";
  const path home = eda::env::getHomePath();
  const path file = home / dir / filename;

  GraphMlParser parser;
  const auto subnetID = parser.parse(file.string())->make();

  const auto &subnet = Subnet::get(subnetID);
  CutExtractor cutExtractor(&subnet, 6);
}

} // namespace eda::gate::optimizer
