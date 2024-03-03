//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger2/sat_checker2.h"

#include "gtest/gtest.h"

using namespace eda::gate::model;

namespace eda::gate::debugger2 {CellTypeID customFourInANDCellType() {
  CellProperties props(true,
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
      "LibCell", libSubnet.make(), model::makeCellTypeAttr(),
      eda::gate::model::CellSymbol::CELL,
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

debugger2::SatChecker2& checker = debugger2::SatChecker2::get();
EXPECT_TRUE(checker.equivalent(Subnet::get(equalSubnetBuilder.make()),
    Subnet::get(genOneCellMappedSubnet(customFourInANDCellType())), map).equal());
}
} // namespace eda::gate::debugger2
