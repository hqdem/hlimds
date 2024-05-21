//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/sat_checker.h"

#include "gtest/gtest.h"

namespace eda::gate::debugger {

static model::CellTypeID customAnd4CellType() {
  static constexpr uint16_t nIn = 4;
  static constexpr uint16_t nOut = 1;

  model::SubnetBuilder builder;
  const auto inputs = builder.addInputs(nIn);
  const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);
  const auto andLink1 = builder.addCell(model::AND, inputs[2], inputs[3]);
  const auto andLink2 = builder.addCell(model::AND, andLink0, andLink1);
  builder.addOutput(andLink2);

  const auto cellTypeID = eda::gate::model::makeCellType(
      model::UNDEF,
      "LibCell",
      builder.make(),
      model::makeCellTypeAttr(),
      model::CellProperties{1, 0, 1, 0, 0, 0, 0, 0, 0},
      nIn,
      nOut);

  return cellTypeID;
}

static model::SubnetID genOneCellMappedSubnet(model::CellTypeID cellTypeID) {
  model::SubnetBuilder builder;
  const auto nIn = model::CellType::get(cellTypeID).getInNum();
  const auto inputs = builder.addInputs(nIn);
  const auto result = builder.addCell(cellTypeID, inputs);
  builder.addOutput(result);
  return builder.make();
}

TEST(SatChecker, Custom4InputsSingleCellTest) {
  model::SubnetBuilder builder;
  const model::Subnet::LinkList inputs = builder.addInputs(4);
  const auto andLink0 = builder.addCell(model::AND, inputs[0], inputs[1]);;
  const auto andLink1 = builder.addCell(model::AND, inputs[2], inputs[3]);
  const auto andLink2 = builder.addCell(model::AND, andLink0, andLink1);
  builder.addOutput(andLink2);

  const auto referenceID = genOneCellMappedSubnet(customAnd4CellType());

  debugger::SatChecker &checker = debugger::SatChecker::get();
  EXPECT_TRUE(checker.areEquivalent(builder.make(), referenceID).equal());
}

/*
        Subnet 1                  Subnet 2
  0   1  2   3   4   5       0  1   2 3   4  5
   \ /  /   /   /   /         \  \  | |   / / 
    6  /   /   /   /           \  \ | |  / /
     \/   /   /   /              \ \| |/ /
      7  /   /   /                   6
       \/   /   /                    |
        8  /   /                    (7)
         \/   /
          9  /
           \/
           10
            |
          (11)
*/
TEST(SatChecker, CellWithMoreThan5Inputs) {
  static constexpr uint16_t nIn = 6;

  model::SubnetBuilder builder1, builder2;
  model::Subnet::LinkList links1 = builder1.addInputs(nIn);
  model::Subnet::LinkList links2 = builder2.addInputs(nIn);

  links1.push_back(builder1.addCell(model::AND, links1[0], links1[1]));
  links1.push_back(builder1.addCell(model::AND, links1[2], links1[6]));
  links1.push_back(builder1.addCell(model::AND, links1[3], links1[7]));
  links1.push_back(builder1.addCell(model::AND, links1[4], links1[8]));
  links1.push_back(builder1.addCell(model::AND, links1[5], links1[9]));
  builder1.addOutput(links1.back());

  links2.push_back(builder2.addCell(model::AND, links2));
  builder2.addOutput(links2.back());

  const auto subnetID1 = builder1.make();
  const auto subnetID2 = builder2.make();

#ifdef UTOPIA_DEBUG
  const auto &subnet1 = model::Subnet::get(subnetID1);
  const auto &subnet2 = model::Subnet::get(subnetID2); 

  std::cout << subnet1 << std::endl;
  std::cout << subnet2 << std::endl;
#endif // UTOPIA_DEBUG

  debugger::SatChecker &checker = debugger::SatChecker::get();
  EXPECT_TRUE(checker.areEquivalent(subnetID1, subnetID2).equal());
}

} // namespace eda::gate::debugger
