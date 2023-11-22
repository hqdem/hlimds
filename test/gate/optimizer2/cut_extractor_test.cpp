//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/printer/dot.h"
#include "gate/model2/subnet.h"
#include "gate/model2/utils/subnet_random.h"
#include "gate/optimizer2/cut_extractor.h"

#include "gtest/gtest.h"

#include <fstream>

namespace eda::gate::optimizer2 {

using Subnet = model::Subnet;
using SubnetID = model::SubnetID;
using SubnetBuilder = model::SubnetBuilder;
using Link = model::Subnet::Link;
using Cut = optimizer2::CutExtractor::Cut;
using CutsList = CutExtractor::CutsList;

template<std::size_t N>
std::array<std::size_t, N> makeInputs(SubnetBuilder &builder) {
  std::array<std::size_t, N> inputs;
  for (std::size_t i = 0; i < N; ++i) {
    inputs[i] = builder.addCell(model::IN, SubnetBuilder::INPUT);
  }
  return inputs;
}

bool cutsEqual(const Cut &cut1, const Cut &cut2) {
  const auto &cut1CellIdxs = cut1.cellIdxs;
  const auto &cut2CellIdxs = cut2.cellIdxs;
  if (cut1.signature != cut2.signature ||
      cut1CellIdxs.size() != cut2CellIdxs.size()) {
    return false;
  }
  for (const auto cellIdx: cut1CellIdxs) {
    if (cut2CellIdxs.find(cellIdx) == cut2CellIdxs.end()) {
      return false;
    }
  }
  return true;
}

bool cutsSetsEqual(const CutsList &cuts1, const CutsList &cuts2) {

  if (cuts1.size() != cuts2.size()) {
    return false;
  }
  for (const Cut &cut1 : cuts1) {
    bool foundEqualCut = false;
    for (std::size_t i = 0; i < cuts2.size(); ++i) {
      if (cutsEqual(cut1, cuts2[i])) {
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
    if (!cutsSetsEqual(cutExtractor.getCuts(i), cellToCuts[i]) ||
        !cutsSetsEqual(cellToCuts[i], cutExtractor.getCuts(i))) {

      return false;
    }
  }
  return true;
}

TEST(CutExtractorTest, OneAND) {
  SubnetBuilder builder;

  std::array<std::size_t, 2> inputs = makeInputs<2>(builder);
  std::size_t andIdx0 = builder.addCell(model::AND,
                                        { Link(inputs[0]), Link(inputs[1]) });
  builder.addCell(model::OUT, Link(andIdx0), SubnetBuilder::OUTPUT);
  Subnet subnet = Subnet::get(builder.make());

  CutExtractor cutExtractor(subnet, 10);
  const std::vector<CutsList> validRes {
    { Cut(1, { 0 }) },
    { Cut(2, { 1 }) },
    { Cut(4, { 2 }), Cut(3, { 0, 1 }) },
    { Cut(8, { 3 }), Cut(4, { 2 }), Cut(3, { 1, 0 }) }
  };
  EXPECT_TRUE(resultValid(cutExtractor, validRes));
}

TEST(CutExtractorTest, TwoAND) {
  SubnetBuilder builder;

  std::array<std::size_t, 3> inputs = makeInputs<3>(builder);
  std::size_t andIdx0 = builder.addCell(model::AND,
                                        { Link(inputs[0]), Link(inputs[1]) });
  std::size_t andIdx1 = builder.addCell(model::AND,
                                        { Link(andIdx0), Link(inputs[2]) });
  builder.addCell(model::OUT, Link(andIdx1), SubnetBuilder::OUTPUT);
  Subnet subnet = Subnet::get(builder.make());

  CutExtractor cutExtractor(subnet, 10);
  const std::vector<CutsList> validRes {
    { Cut(1, { 0 }) },
    { Cut(2, { 1 }) },
    { Cut(4, { 2 }) },
    { Cut(8, { 3 }), Cut(3, { 0, 1 }) },
    { Cut(16, { 4 }), Cut(12, { 3, 2 }), Cut(7, { 0, 1, 2 }) },
    { Cut(32, { 5 }), Cut(16, { 4 }), Cut(12, { 3, 2 }),
      Cut(7, { 0, 1, 2 }) }
  };
  EXPECT_TRUE(resultValid(cutExtractor, validRes));
}

TEST(CutExtractorTest, Domination) {
  SubnetBuilder builder;

  std::array<std::size_t, 2> inputs = makeInputs<2>(builder);
  std::size_t andIdx0 = builder.addCell(model::AND,
                                        { Link(inputs[0]), Link(inputs[1]) });
  std::size_t notIdx0 = builder.addCell(model::NOT,
                                        Link(andIdx0));
  std::size_t andIdx1 = builder.addCell(model::AND,
                                        { Link(andIdx0), Link(notIdx0) });
  builder.addCell(model::OUT, Link(andIdx1), SubnetBuilder::OUTPUT);
  Subnet subnet = Subnet::get(builder.make());

  CutExtractor cutExtractor(subnet, 10);
  const std::vector<CutsList> validRes {
    { Cut(1, { 0 }) },
    { Cut(2, { 1 }) },
    { Cut(4, { 2 }), Cut(3, { 1, 0 }) },
    { Cut(8, { 3 }), Cut(4, { 2 }), Cut(3, { 1, 0 }) },
    { Cut(16, { 4 }), Cut(4, { 2 }), Cut(3, { 1, 0 }) },
    { Cut(32, { 5 }), Cut(16, { 4 }), Cut(4, { 2 }), Cut(3, { 1, 0 }) }
  };
  EXPECT_TRUE(resultValid(cutExtractor, validRes));
}

TEST(CutExtractorTest, LimitedK) {
  SubnetBuilder builder;

  std::array<std::size_t, 1> inputs = makeInputs<1>(builder);
  std::size_t notIdx0 = builder.addCell(model::NOT, Link(inputs[0]));
  std::size_t notIdx1 = builder.addCell(model::NOT, Link(inputs[0]));
  std::size_t notIdx2 = builder.addCell(model::NOT, Link(notIdx0));
  std::size_t notIdx3 = builder.addCell(model::NOT, Link(notIdx0));
  std::size_t notIdx4 = builder.addCell(model::NOT, Link(notIdx1));
  std::size_t notIdx5 = builder.addCell(model::NOT, Link(notIdx1));
  std::size_t andIdx0 = builder.addCell(model::AND,
                                        { Link(notIdx2), Link(notIdx3) });
  std::size_t andIdx1 = builder.addCell(model::AND,
                                        { Link(notIdx4), Link(notIdx5) });
  std::size_t andIdx2 = builder.addCell(model::AND,
                                        { Link(andIdx0), Link(andIdx1) });
  builder.addCell(model::OUT, Link(andIdx2), SubnetBuilder::OUTPUT);
  Subnet subnet = Subnet::get(builder.make());

  CutExtractor cutExtractor(subnet, 2);
  const std::vector<CutsList> validRes {
    { Cut(1, { 0 }) },
    { Cut(2, { 1 }), Cut(1, { 0 }) },
    { Cut(4, { 2 }), Cut(1, { 0 }) },
    { Cut(8, { 3 }), Cut(2, { 1 }), Cut(1, { 0 }) },
    { Cut(16, { 4 }), Cut(2, { 1 }), Cut(1, { 0 }) },
    { Cut(32, { 5 }), Cut(4, { 2 }), Cut(1, { 0 }) },
    { Cut(64, { 6 }), Cut(4, { 2 }), Cut(1, { 0 }) },
    { Cut(128, { 7 }), Cut(24, { 3, 4 }), Cut(2, { 1 }), Cut(1, { 0 }) },
    { Cut(256, { 8 }), Cut(96, { 5, 6 }), Cut(4, { 2 }), Cut(1, { 0 }) },
    { Cut(512, { 9 }), Cut(384, { 7, 8 }), Cut(132, { 7, 2 }),
      Cut(258, { 8, 1 }), Cut(6, { 1, 2 }), Cut(1, { 0 }) },
    { Cut(1024, { 10 }), Cut(512, { 9 }), Cut(384, { 7, 8 }),
      Cut(132, { 7, 2 }), Cut(258, { 8, 1 }), Cut(6, { 1, 2 }),
      Cut(1, { 0 }) }
  };
  EXPECT_TRUE(resultValid(cutExtractor, validRes));
}

TEST(CutExtractorTest, CutsIntersection) {
  SubnetBuilder builder;
  std::array<std::size_t, 3> inputs = makeInputs<3>(builder);

  std::size_t andIdx0 = builder.addCell(model::AND,
                                        { Link(inputs[0]), Link(inputs[1]) });
  std::size_t andIdx1 = builder.addCell(model::AND,
                                        { Link(inputs[1]), Link(inputs[2]) });
  std::size_t andIdx2 = builder.addCell(model::AND,
                                        { Link(andIdx0), Link(andIdx1) });
  builder.addCell(model::OUT, Link(andIdx2), SubnetBuilder::OUTPUT);
  Subnet subnet = Subnet::get(builder.make());

  CutExtractor cutExtractor(subnet, 3);
  const std::vector<CutsList> validRes {
    { Cut(1, { 0 }) },
    { Cut(2, { 1 }) },
    { Cut(4, { 2 }) },
    { Cut(8, { 3 }), Cut(3, { 1, 0 }) },
    { Cut(16, { 4 }), Cut(6, { 1, 2 }) },
    { Cut(32, { 5 }), Cut(24, { 3, 4 }), Cut(14, { 3, 1, 2 }),
      Cut(19, { 4, 0, 1 }), Cut(7, { 0, 1, 2 }) },
    { Cut(64, { 6 }), Cut(32, { 5 }), Cut(24, { 3, 4 }), Cut(14, { 3, 1, 2 }),
      Cut(19, { 4, 0, 1 }), Cut(7, { 0, 1, 2 }) }
  };
  EXPECT_TRUE(resultValid(cutExtractor, validRes));
}

TEST(CutExtractorTest, NoCuts) {
  SubnetBuilder builder;
  std::array<std::size_t, 3> inputs = makeInputs<3>(builder);

  std::size_t andIdx0 = builder.addCell(model::AND,
                                        { Link(inputs[0]), Link(inputs[1]),
                                          Link(inputs[2]) });
  builder.addCell(model::OUT, Link(andIdx0), SubnetBuilder::OUTPUT);
  Subnet subnet = Subnet::get(builder.make());

  CutExtractor cutExtractor(subnet, 2);
  const std::vector<CutsList> validRes {
    { Cut(1, { 0 }) },
    { Cut(2, { 1 }) },
    { Cut(4, { 2 }) },
    { Cut(8, { 3 }) },
    { Cut(16, { 4 }), Cut(8, { 3 }) }
  };
  EXPECT_TRUE(resultValid(cutExtractor, validRes));
}

TEST(CutExtractorTest, SameElementsInCuts) {
  SubnetBuilder builder;
  std::array<std::size_t, 1> inputs = makeInputs<1>(builder);

  std::size_t notIdx0 = builder.addCell(model::NOT, Link(inputs[0]));
  std::size_t notIdx1 = builder.addCell(model::NOT, Link(inputs[0]));
  std::size_t andIdx0 = builder.addCell(model::AND,
                                        { Link(notIdx0), Link(notIdx1) });
  builder.addCell(model::OUT, Link(andIdx0), SubnetBuilder::OUTPUT);
  Subnet subnet = Subnet::get(builder.make());

  CutExtractor cutExtractor(subnet, 1);
  const std::vector<CutsList> validRes {
    { Cut(1, { 0 }) },
    { Cut(2, { 1 }), Cut(1, { 0 }) },
    { Cut(4, { 2 }), Cut(1, { 0 }) },
    { Cut(8, { 3 }), Cut(1, { 0 }) },
    { Cut(16, { 4 }), Cut(8, { 3 }), Cut(1, { 0 }) }
  };
  EXPECT_TRUE(resultValid(cutExtractor, validRes));
}

TEST(CutExtractorTest, LargeSubnet) {
  const auto subnetID = model::randomSubnet(1, 1, 10000, 2, 3);
  Subnet subnet = Subnet::get(subnetID);
  CutExtractor cutExtractor(subnet, 3);
}

} // namespace eda::gate::optimizer2
