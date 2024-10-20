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
  auto builder = std::make_shared<SubnetBuilder>();
  auto links = builder->addInputs(2);
  links.push_back(builder->addCell(CellSymbol::ONE));

  links.push_back(builder->addCell(CellSymbol::AND, links[0], links[1]));
  links.push_back(builder->addCell(CellSymbol::AND, links[1], links[2]));
  links.push_back(builder->addCell(CellSymbol::AND, links[3], links[4]));
  builder->addOutput(links[3]);
  builder->addOutput(links.back());

  model::EntryIDList cut{0, 1};
  model::EntryID rootID = 5;

  const auto view = getMffc(builder, rootID, cut);

  const auto &viewIns  = view.getInputs();
  const auto &viewOuts = view.getOutputs();

  // Check MFFC.
  EXPECT_EQ(viewIns.size(), 2);
  EXPECT_TRUE(((viewIns[0].idx == 1) && (viewIns[1].idx == 3)) ||
              ((viewIns[0].idx == 3) && (viewIns[1].idx == 1)));
  EXPECT_TRUE((viewOuts.size() == 1) && (viewOuts[0].idx == rootID));
  // Check refcounts.
  EXPECT_TRUE((builder->getCell(0).refcount == 1) &&
              (builder->getCell(1).refcount == 2) &&
              (builder->getCell(2).refcount == 1) &&
              (builder->getCell(3).refcount == 2) &&
              (builder->getCell(4).refcount == 1) &&
              (builder->getCell(5).refcount == 1));
}

TEST(MffcTest, CutBound2) {
  using CellSymbol    = eda::gate::model::CellSymbol;
  using Link          = eda::gate::model::Subnet::Link;
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
  auto builder = std::make_shared<SubnetBuilder>();
  auto links = builder->addInputs(2);
  links.push_back(builder->addCell(CellSymbol::ONE));

  links.push_back(builder->addCell(CellSymbol::AND, links[0], links[1]));
  links.push_back(builder->addCell(CellSymbol::AND, links[1], links[2]));
  links.push_back(builder->addCell(CellSymbol::AND, links[3], links[4]));
  builder->addOutput(links.back());

  model::Subnet::LinkList cut{Link(3), Link(4)};
  model::EntryID rootID = 5;

  const auto view = getMffc(builder, rootID, {cut[0].idx, cut[1].idx});
  EXPECT_EQ(view.getInputs(), cut);
  EXPECT_EQ(view.getOutputs()[0].idx, rootID);
  // Check refcounts.
  EXPECT_TRUE((builder->getCell(3).refcount == 1) &&
              (builder->getCell(4).refcount == 1) &&
              (builder->getCell(5).refcount == 1));
}

TEST(MffcTest, DepthBound1) {
  using CellSymbol    = eda::gate::model::CellSymbol;
  using EntryID       = eda::gate::model::EntryID;
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
  auto builder = std::make_shared<SubnetBuilder>();
  auto links = builder->addInputs(5);
  links.push_back(builder->addCell(CellSymbol::AND, links[3], links[4]));
  links.push_back(builder->addCell(CellSymbol::AND, links[2], links[5]));
  links.push_back(builder->addCell(CellSymbol::AND, links[1], links[6]));
  links.push_back(builder->addCell(CellSymbol::BUF, links[0]));
  links.push_back(builder->addCell(CellSymbol::AND, links[7], links[8]));
  builder->addOutput(links.back());

  size_t maxDepth = 3;
  model::EntryID rootID = 9;

  const auto view = getMffc(builder, rootID, maxDepth);

  const auto &viewIns  = view.getInputs();
  const auto &viewOuts = view.getOutputs();
  // Check MFFC.
  EXPECT_EQ(viewIns.size(), 4);
  std::set<EntryID> cut;
  for (const auto &in : viewIns) {
    cut.insert(in.idx);
  }
  std::set<EntryID> check{0, 1, 2, 5};
  EXPECT_EQ(cut, check);
  EXPECT_TRUE((viewOuts.size() == 1) && (viewOuts[0].idx == rootID));
  // Check refcounts.
  for (SafePasser iter = builder->begin();
       !builder->getCell(*iter).isOut(); ++iter) {

    EXPECT_EQ(builder->getCell(*iter).refcount, 1);
  }
}

TEST(MffcTest, DepthBound2) {
  using CellSymbol    = eda::gate::model::CellSymbol;
  using EntryID       = eda::gate::model::EntryID;
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
  const size_t nLoops = 19;
  const size_t maxDepth = 3;
  const EntryID rootId = 20;

  auto builder = std::make_shared<SubnetBuilder>();
  auto links = builder->addInputs(nInputs);

  size_t skip = nInputs - 1;
  size_t line = skip;
  for (size_t i = 0; i < nLoops; ++i) {
    if (i == skip) {
      skip += line;
      line--;
      continue;
    }
    const auto link = builder->addCell(CellSymbol::AND, {links[i], links[i+1]});
    links.push_back(link);
  }
  builder->addOutput(links.back());

  const auto view = getMffc(builder, rootId, maxDepth);

  const std::set<EntryID> check{11, 12, 13, 14};
  std::set<EntryID> cut;
  for (const auto &in : view.getInputs()) {
    cut.insert(in.idx);
  }

  EXPECT_EQ(view.getInputs().size(), 4);
  EXPECT_EQ(cut, check);
  EXPECT_EQ(view.getOutputs()[0].idx, rootId);
}

} // namespace eda::gate::optimizer
