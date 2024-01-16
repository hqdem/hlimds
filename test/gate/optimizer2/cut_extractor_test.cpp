//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/utils/subnet_random.h"
#include "gate/optimizer2/cut_extractor.h"

#include "gate/optimizer2/cone_builder.h"

#include "gtest/gtest.h"

#include <fstream>

namespace eda::gate::optimizer2 {

using Subnet = model::Subnet;
using SubnetID = model::SubnetID;
using SubnetBuilder = model::SubnetBuilder;
using Link = model::Subnet::Link;
using Cut = optimizer2::CutExtractor::Cut;
using CutsEntries = CutExtractor::CutsEntries;
using CutsList = CutExtractor::CutsList;

template<std::size_t N>
std::array<std::size_t, N> makeInputs(SubnetBuilder &builder) {
  std::array<std::size_t, N> inputs;
  for (std::size_t i = 0; i < N; ++i) {
    inputs[i] = builder.addInput();
  }
  return inputs;
}

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

  auto inputs = makeInputs<2>(builder);
  std::size_t andIdx0 = builder.addCell(model::AND,
                                        { Link(inputs[0]), Link(inputs[1]) });
  builder.addOutput(Link(andIdx0));
  Subnet subnet = Subnet::get(builder.make());

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

  auto inputs = makeInputs<3>(builder);
  std::size_t andIdx0 = builder.addCell(model::AND,
                                        { Link(inputs[0]), Link(inputs[1]) });
  std::size_t andIdx1 = builder.addCell(model::AND,
                                        { Link(andIdx0), Link(inputs[2]) });
  builder.addOutput(Link(andIdx1));
  Subnet subnet = Subnet::get(builder.make());

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

  auto inputs = makeInputs<2>(builder);
  std::size_t andIdx0 = builder.addCell(model::AND,
                                        { Link(inputs[0]), Link(inputs[1]) });
  std::size_t notIdx0 = builder.addCell(model::BUF,
                                        Link(andIdx0, true));
  std::size_t andIdx1 = builder.addCell(model::AND,
                                        { Link(andIdx0), Link(notIdx0) });
  builder.addOutput(Link(andIdx1));
  Subnet subnet = Subnet::get(builder.make());

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

  std::array<std::size_t, 1> inputs = makeInputs<1>(builder);
  std::size_t notIdx0 = builder.addCell(model::BUF, Link(inputs[0], true));
  std::size_t notIdx1 = builder.addCell(model::BUF, Link(inputs[0], true));
  std::size_t notIdx2 = builder.addCell(model::BUF, Link(notIdx0,   true));
  std::size_t notIdx3 = builder.addCell(model::BUF, Link(notIdx0,   true));
  std::size_t notIdx4 = builder.addCell(model::BUF, Link(notIdx1,   true));
  std::size_t notIdx5 = builder.addCell(model::BUF, Link(notIdx1,   true));
  std::size_t andIdx0 = builder.addCell(model::AND,
                                        { Link(notIdx2), Link(notIdx3) });
  std::size_t andIdx1 = builder.addCell(model::AND,
                                        { Link(notIdx4), Link(notIdx5) });
  std::size_t andIdx2 = builder.addCell(model::AND,
                                        { Link(andIdx0), Link(andIdx1) });
  builder.addOutput(Link(andIdx2));
  Subnet subnet = Subnet::get(builder.make());

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
  auto inputs = makeInputs<3>(builder);

  std::size_t andIdx0 = builder.addCell(model::AND,
                                        { Link(inputs[0]), Link(inputs[1]) });
  std::size_t andIdx1 = builder.addCell(model::AND,
                                        { Link(inputs[1]), Link(inputs[2]) });
  std::size_t andIdx2 = builder.addCell(model::AND,
                                        { Link(andIdx0), Link(andIdx1) });
  builder.addOutput(Link(andIdx2));
  Subnet subnet = Subnet::get(builder.make());

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
  auto inputs = makeInputs<3>(builder);

  std::size_t andIdx0 = builder.addCell(model::AND,
                                        { Link(inputs[0]), Link(inputs[1]),
                                          Link(inputs[2]) });
  builder.addOutput(Link(andIdx0));
  Subnet subnet = Subnet::get(builder.make());

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
  auto inputs = makeInputs<1>(builder);

  std::size_t notIdx0 = builder.addCell(model::BUF, Link(inputs[0], true));
  std::size_t notIdx1 = builder.addCell(model::BUF, Link(inputs[0], true));
  std::size_t andIdx0 = builder.addCell(model::AND,
                                        { Link(notIdx0), Link(notIdx1) });
  builder.addOutput(Link(andIdx0));
  Subnet subnet = Subnet::get(builder.make());

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
  auto inputs = makeInputs<6>(builder);

  std::size_t andIdx0 = builder.addCell(model::AND,
                                        { Link(inputs[0]), Link(inputs[1]),
                                          Link(inputs[2]), Link(inputs[3]),
                                          Link(inputs[4]), Link(inputs[5]) });
  builder.addOutput(Link(andIdx0));
  Subnet subnet = Subnet::get(builder.make());

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

  auto inputs = makeInputs<2>(builder);
  std::size_t andIdx0 = builder.addCell(model::AND,
                                        { Link(inputs[0]), Link(inputs[1]) });
  builder.addOutput(Link(andIdx0));
  Subnet subnet = Subnet::get(builder.make());

  CutExtractor cutExtractor(&subnet, 2);
  const std::vector<CutsEntries> validRes {
    { { 0 } },
    { { 1 } },
    { { 2 }, { 0, 1 } },
    { { 3 }, { 2 }, { 1, 0 } }
  };

  for (std::size_t i = 0; i < subnet.getEntries().size(); ++i) {
    EXPECT_TRUE(validRes[i] == cutExtractor.getCutsEntries(i));
  }
}

TEST(CutExtractorTest, LargeSubnet) {
  const auto subnetID = model::randomSubnet(1, 1, 10000, 2, 3);
  Subnet subnet = Subnet::get(subnetID);
  CutExtractor cutExtractor(&subnet, 3);
}

TEST(CutExtractorTest, bugTest) {
SubnetBuilder builder;

auto inputs = makeInputs<4>(builder);
std::size_t andIdx0 = builder.addCell(model::AND,
                                      { Link(inputs[0]), Link(inputs[1]) });
std::size_t andIdx1 = builder.addCell(model::AND,
                                      { Link(inputs[2]), Link(inputs[3]) });
std::size_t andIdx2 = builder.addCell(model::AND,
                                      { Link(andIdx0), Link(andIdx1) });
builder.addCell(model::OUT, Link(andIdx2), SubnetBuilder::OUTPUT);
SubnetID subID = builder.make();
Subnet subnet = Subnet::get(subID);

CutExtractor cutExtractor(&subnet, 6);
eda::gate::optimizer2::ConeBuilder coneBuilder(&subnet);
for (const auto &cut : cutExtractor.getCuts(andIdx2)) {
  SubnetID coneSubnetID = coneBuilder.getCone(cut).subnetID;
  std::cout <<  model::Subnet::get(coneSubnetID) << std::endl;
}
}

} // namespace eda::gate::optimizer2
