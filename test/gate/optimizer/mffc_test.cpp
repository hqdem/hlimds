//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/celltype.h"
#include "gate/optimizer/mffc.h"
#include "gate/optimizer/safe_passer.h"

#include "gtest/gtest.h"

#include <set>

namespace eda::gate::optimizer {

TEST(MffcTest, CutBound1) {
  using CellSymbol    = eda::gate::model::CellSymbol;
  using SubnetBuilder = eda::gate::model::SubnetBuilder;
  /*
  * in(0)  in(1) CONST
  *     \ /    \ /
  *      3      4
  *      | \  /
  *      |  5          - Root
  *      |  |
  *     out out
  */
  SubnetBuilder builder;
  auto links = builder.addInputs(2);
  links.push_back(builder.addCell(CellSymbol::ONE));

  links.push_back(builder.addCell(CellSymbol::AND, links[0], links[1]));
  links.push_back(builder.addCell(CellSymbol::AND, links[1], links[2]));
  links.push_back(builder.addCell(CellSymbol::AND, links[3], links[4]));
  builder.addOutput(links[3]);
  builder.addOutput(links.back());

  std::vector<size_t> cut{0, 1};
  size_t rootID = 5;

  const auto view = getMffc(builder, rootID, cut);

  const std::vector<size_t> &viewIns  = view.getInputs();
  const std::vector<size_t> &viewOuts = view.getOutputs();

  // Check MFFC.
  EXPECT_EQ(viewIns.size(), 2);
  EXPECT_TRUE(((viewIns[0] == 1) && (viewIns[1] == 3)) ||
              ((viewIns[0] == 3) && (viewIns[1] == 1)));
  EXPECT_TRUE((viewOuts.size() == 1) && (viewOuts[0] == rootID));
  // Check refcounts.
  EXPECT_TRUE((builder.getCell(0).refcount == 1) &&
              (builder.getCell(1).refcount == 2) &&
              (builder.getCell(2).refcount == 1) &&
              (builder.getCell(3).refcount == 2) &&
              (builder.getCell(4).refcount == 1) &&
              (builder.getCell(5).refcount == 1));
}

TEST(MffcTest, CutBound2) {
  using CellSymbol    = eda::gate::model::CellSymbol;
  using SubnetBuilder = eda::gate::model::SubnetBuilder;
  /*
  * in(0)  in(1) CONST
  *     \ /    \ /
  *      3      4
  *        \  /
  *         5          - Root
  *         |
  *        out
  */
  SubnetBuilder builder;
  auto links = builder.addInputs(2);
  links.push_back(builder.addCell(CellSymbol::ONE));

  links.push_back(builder.addCell(CellSymbol::AND, links[0], links[1]));
  links.push_back(builder.addCell(CellSymbol::AND, links[1], links[2]));
  links.push_back(builder.addCell(CellSymbol::AND, links[3], links[4]));
  builder.addOutput(links.back());

  std::vector<size_t> cut{3, 4};
  size_t rootID = 5;

  const auto view = getMffc(builder, rootID, cut);
  EXPECT_EQ(view.getInputs(), cut);
  EXPECT_EQ(view.getOutputs()[0], rootID);
  // Check refcounts.
  EXPECT_TRUE((builder.getCell(3).refcount == 1) &&
              (builder.getCell(4).refcount == 1) &&
              (builder.getCell(5).refcount == 1));
}

TEST(MffcTest, DepthBound1) {
  using CellSymbol    = eda::gate::model::CellSymbol;
  using SubnetBuilder = eda::gate::model::SubnetBuilder;
  /*
  *          in3 in4
  *             \/
  *        in2  5
  *           \/
  *   in0 in1 6 
  *    |   \ / 
  *    8   7
  *     \ /
  *      9             - Root
  *      |
  *     out
  */
  SubnetBuilder builder;
  auto links = builder.addInputs(5);
  links.push_back(builder.addCell(CellSymbol::AND, links[3], links[4]));
  links.push_back(builder.addCell(CellSymbol::AND, links[2], links[5]));
  links.push_back(builder.addCell(CellSymbol::AND, links[1], links[6]));
  links.push_back(builder.addCell(CellSymbol::BUF, links[0]));
  links.push_back(builder.addCell(CellSymbol::AND, links[7], links[8]));
  builder.addOutput(links.back());

  size_t maxDepth = 3;
  size_t rootID = 9;

  const auto view = getMffc(builder, rootID, maxDepth);

  const std::vector<size_t> &viewIns  = view.getInputs();
  const std::vector<size_t> &viewOuts = view.getOutputs();
  // Check MFFC.
  EXPECT_EQ(viewIns.size(), 4);
  std::set<size_t> cut(viewIns.begin(), viewIns.end());
  std::set<size_t> check{0, 1, 2, 5};
  EXPECT_EQ(cut, check);
  EXPECT_TRUE((viewOuts.size() == 1) && (viewOuts[0] == rootID));
  // Check refcounts.
  for (SafePasser iter = builder.begin();
       !builder.getCell(*iter).isOut(); ++iter) {

    EXPECT_EQ(builder.getCell(*iter).refcount, 1);
  }
}

TEST(MffcTest, DepthBound2) {
  using CellSymbol    = eda::gate::model::CellSymbol;
  using SubnetBuilder = eda::gate::model::SubnetBuilder;
  /*
  *   0  1  2  3  4  5 - Inputs
  *    \/ \/ \/ \/ \/
  *    6  7  8  9  10
  *     \/ \/ \/ \/
  *     11 12 13 14    - Cut
  *      \/ \/ \/
  *      15 16 17
  *       \/ \/
  *       18 19
  *        \/
  *        20          - Root
  *        |
  *       out
  */
  const size_t nInputs = 6;
  const size_t rootId = 20;
  const size_t nLoops = 19;
  const size_t maxDepth = 3;

  SubnetBuilder builder;
  auto links = builder.addInputs(nInputs);

  size_t skip = nInputs - 1;
  size_t line = skip;
  for (size_t i = 0; i < nLoops; ++i) {
    if (i == skip) {
      skip += line;
      line--;
      continue;
    }
    const auto link = builder.addCell(CellSymbol::AND, {links[i], links[i+1]});
    links.push_back(link);
  }
  builder.addOutput(links.back());

  const auto view = getMffc(builder, rootId, maxDepth);

  const std::set<size_t> check{11, 12, 13, 14};
  const std::set<size_t> cut(view.getInputs().begin(), view.getInputs().end());

  EXPECT_EQ(cut, check);
  EXPECT_EQ(view.getOutputs(), std::vector<size_t>{rootId});
}

} // namespace eda::gate::optimizer
