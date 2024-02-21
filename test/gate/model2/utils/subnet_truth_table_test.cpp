//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/utils/subnet_truth_table.h"

#include "gtest/gtest.h"

namespace eda::gate::model {

void checkComputeCare(const Subnet &subnet,
                      const std::string &care,
                      int32_t nVars) {

  const auto computed = computeCare(subnet);

  kitty::dynamic_truth_table careTable(nVars);
  kitty::create_from_binary_string(careTable, care);

  EXPECT_EQ(computed, careTable);
}

TEST(ComputeCare, EqualOuts) {
  // out0 == out1
  using LinkList = Subnet::LinkList;

  SubnetBuilder builder;

  LinkList links = builder.addInputs(3);
  builder.addOutputs(LinkList({links[0], links[0]}));

  checkComputeCare(Subnet::get(builder.make()), "1001", 2);
}

TEST(ComputeCare, SimpleTest) {
  // care = 0b1101
  using LinkList = Subnet::LinkList;

  SubnetBuilder builder;

  LinkList links = builder.addInputs(3);
  links.push_back(builder.addCell(CellSymbol::AND, {links[0], links[1]}));
  links.push_back(builder.addCell(CellSymbol::AND, {links.back(), links[2]}));
  links.push_back(builder.addCell(CellSymbol::OR, {links.back(), links[2]}));

  builder.addOutputs({links[links.size() - 2], links.back()});

  checkComputeCare(Subnet::get(builder.make()), "1101", 2);
}

} // namespace eda::gate::model
