//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/celltype.h"
#include "gate/optimizer/reconvergence.h"

#include "gtest/gtest.h"

namespace eda::gate::optimizer {

TEST(ReconvergenceTest, CorrectnessTest) {
  using CellSymbol    = eda::gate::model::CellSymbol;
  using Link          = eda::gate::model::Subnet::Link;
  using LinkList      = eda::gate::model::Subnet::LinkList;
  using SubnetBuilder = eda::gate::model::SubnetBuilder;
  /*
  * in(0)  in(1) CONST - Inputs
  *     \ /    \ /
  *      3      4
  *        \ /
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

  const auto cutView = getReconvergentCut(builder, 5, 4);

  const LinkList check = {Link(1), Link(0)};

  EXPECT_EQ(check, cutView.getInputs());
  EXPECT_EQ(LinkList{Link(5)}, cutView.getOutputs());
}

TEST(ReconvergenceTest, SimpleTest) {
  using CellSymbol    = eda::gate::model::CellSymbol;
  using Link          = eda::gate::model::Subnet::Link;
  using LinkList      = eda::gate::model::Subnet::LinkList;
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
  const size_t cutSize = 4;
  const size_t nInputs = 6;
  const size_t nLoops = 19;
  const model::EntryID rootId = 20;

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

  const auto cutView = getReconvergentCut(builder, rootId, cutSize);

  const LinkList check = {Link(11), Link(12), Link(13), Link(14)};

  EXPECT_EQ(cutView.getInputs(), check);
  EXPECT_EQ(cutView.getOutputs(), LinkList{Link(rootId)});
}

} // namespace eda::gate::optimizer
