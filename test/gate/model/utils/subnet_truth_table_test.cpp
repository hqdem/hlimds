//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/utils/subnet_truth_table.h"

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

TEST(ComputeCare, ConstTest) {
  // care = 0b10
  using LinkList = Subnet::LinkList;

  SubnetBuilder constBuilder;

  const auto link1 = constBuilder.addCell(CellSymbol::ONE);
  constBuilder.addOutput(link1);
  const auto attrID = model::makeCellTypeAttr();
  const auto cellTypeID = model::makeCellType(
    model::CellSymbol::UNDEF,
    "cell_one",
    constBuilder.make(),
    attrID,
    model::CellProperties{1, 0, 1, 1, 0, 0, 0, 0, 0},
    0,
    1);

  SubnetBuilder builder;

  LinkList links = builder.addInputs(1);
  links.push_back(builder.addCell(cellTypeID,{}));
  links.push_back(builder.addCell(CellSymbol::AND, links[0], links[1]));

  builder.addOutputs({links.back()});
  const auto &subnet = Subnet::get(builder.make());
  std::cout << subnet << std::endl;

  EXPECT_EQ(kitty::to_binary(model::evaluate(subnet)[0]) , "10" );
}

} // namespace eda::gate::model
