//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/celltype.h"
#include "gate/optimizer2/reconvergence_cut.h"

#include "gtest/gtest.h"

namespace eda::gate::optimizer2 {

TEST(ReconvergenceCutTest, Simple) {
  using CellSymbol    = eda::gate::model::CellSymbol;
  using Subnet        = eda::gate::model::Subnet;
  using SubnetBuilder = eda::gate::model::SubnetBuilder;
  /*
  *   1  2  3  4  5  6 - Inputs
  *    \/ \/ \/ \/ \/
  *    7  8  9  10 11
  *     \/ \/ \/ \/
  *     12 13 14 15    - Cut
  *      \/ \/ \/
  *      16 17 18
  *       \/ \/
  *       19 20
  *        \/
  *        21          - Root
  *        |
  *       out
  */
  const size_t cutSize = 4;
  const size_t nInputs = 6;
  const size_t rootId = 21;
  const size_t nLoops = 19;

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
  
  const auto &subnet = Subnet::get(builder.make());
  const auto cut = getReconvergenceCut(subnet, rootId, cutSize);
  
  const std::vector<uint32_t> check = {11, 12, 13, 14};

  EXPECT_EQ(cut, check);
}

} // namespace eda::gate::optimizer2
