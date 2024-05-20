//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/sat_checker.h"

#include "gtest/gtest.h"

using namespace eda::gate::model;

namespace eda::gate::debugger {

using Link = model::Subnet::Link;
using LinkList = model::Subnet::LinkList;

CellTypeID customFourInANDCellType() {
  CellProperties props(true,
                       false,
                       true,
                       false,
                       false,
                       false,
                       false,
                       false,
                       false);

  SubnetBuilder libSubnet;
  Link idxLib[4];
  for (size_t i = 0; i < 4; ++i) {
    idxLib[i] = libSubnet.addInput();
  }
  auto idxANDLib0 = libSubnet.addCell(AND, idxLib[0], idxLib[1]);
  auto idxANDLib1 = libSubnet.addCell(AND, idxLib[2], idxLib[3]);
  auto idxANDLib2 = libSubnet.addCell(AND, idxANDLib0, idxANDLib1);
  libSubnet.addOutput(Link(idxANDLib2));

  CellTypeID cellTypeID = eda::gate::model::makeCellType(
      eda::gate::model::CellSymbol::UNDEF,
      "LibCell", libSubnet.make(), model::makeCellTypeAttr(),
      props, 4,1);
  return cellTypeID;
}

SubnetID genOneCellMappedSubnet(CellTypeID cellTypeID) {
  SubnetBuilder mappedSubnetBuilder;
  auto nIn = CellType::get(cellTypeID).getInNum();
  LinkList inIdx;
  for (size_t i = 0; i < nIn; ++i) {
    inIdx.emplace_back(mappedSubnetBuilder.addInput());
  }
  auto customCellIdx = mappedSubnetBuilder.addCell(cellTypeID, inIdx);
  mappedSubnetBuilder.addOutput(Link(customCellIdx));

  return mappedSubnetBuilder.make();
}

TEST(SATTest, CustomFourInSingleCellTest) {
  SubnetBuilder equalSubnetBuilder;
  LinkList links;
  for (size_t i = 0; i < 4; ++i) {
    links.emplace_back(equalSubnetBuilder.addInput());
  }

  auto idxANDSubnetOut0 = equalSubnetBuilder.addCell(CELL_TYPE_ID_AND, LinkList{links[0], links[1]});;
  auto idxANDSubnetOut1 = equalSubnetBuilder.addCell(CELL_TYPE_ID_AND, LinkList{links[2], links[3]});
  auto idxANDSubnetOut2 = equalSubnetBuilder.addCell(CELL_TYPE_ID_AND, LinkList{idxANDSubnetOut0, idxANDSubnetOut1});
  equalSubnetBuilder.addOutput(idxANDSubnetOut2);

  std::unordered_map<size_t, size_t> map;

  map[0] = 0;
  map[1] = 1;
  map[2] = 2;
  map[3] = 3;
  map[7] = 5;

  debugger::SatChecker &checker = debugger::SatChecker::get();
  EXPECT_TRUE(checker.areEquivalent(equalSubnetBuilder.make(),
    genOneCellMappedSubnet(customFourInANDCellType()), map).equal());
  }

/*
  subnet1                   subnet2
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
TEST(SATTest, CellWithMoreThan5Inputs) {
  SubnetBuilder subnetBuilder1, subnetBuilder2;
  LinkList links1, links2;

  const size_t n = 6;
  for (size_t i = 0; i < n; ++i) {
    links1.emplace_back(subnetBuilder1.addInput());
    links2.emplace_back(subnetBuilder2.addInput());
  }

  links1.emplace_back(subnetBuilder1.addCell(AND, links1[0], links1[1]));
  links1.emplace_back(subnetBuilder1.addCell(AND, links1[2], links1[6]));
  links1.emplace_back(subnetBuilder1.addCell(AND, links1[3], links1[7]));
  links1.emplace_back(subnetBuilder1.addCell(AND, links1[4], links1[8]));
  links1.emplace_back(subnetBuilder1.addCell(AND, links1[5], links1[9]));
  subnetBuilder1.addOutput(links1[10]);

  auto subnet2Idx1 = subnetBuilder2.addCell(AND, links2);
  subnetBuilder2.addOutput(subnet2Idx1);

  std::unordered_map<size_t, size_t> map;

  const auto subnetID1 = subnetBuilder1.make();
  const auto subnetID2 = subnetBuilder2.make();

  const auto &subnet1 = Subnet::get(subnetID1);
  const auto &subnet2 = Subnet::get(subnetID2); 

  for (size_t i = 0; i < n; ++i) {
    map[i] = i;
  }
  map[subnet1.size() - 1] = subnet2.size() - 1;

#ifdef UTOPIA_DEBUG
  std::cout << subnet1 << std::endl;
  std::cout << subnet2 << std::endl;
#endif // UTOPIA_DEBUG

  debugger::SatChecker &checker = debugger::SatChecker::get();
  EXPECT_TRUE(checker.areEquivalent(subnetID1, subnetID2, map).equal());
}

} // namespace eda::gate::debugger
