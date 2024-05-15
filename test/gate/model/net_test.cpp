//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021-2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/net.h"
#include "gate/model/printer/printer.h"

#include "gtest/gtest.h"

#include <iostream>
#include <vector>

namespace eda::gate::model {

TEST(NetTest, SimpleTest) {
  static constexpr size_t Depth = 16;
  static constexpr size_t Breadth = 8;

  NetBuilder netBuilder;

  std::array<CellID, Breadth> cells[2];
  for (size_t i = 0; i < Breadth; i++) {
    auto cellID = makeCell(IN);
    cells[0][i] = cellID;
    netBuilder.addCell(cellID);
  }

  size_t k = 0;
  for (size_t i = 0; i < Depth; i++) {
    for (size_t j = 0; j < Breadth; j++) {
      auto cellID = makeCell(AND, cells[k][j], cells[k][(Breadth - j) - 1]);
      cells[1 - k][j] = cellID;
      netBuilder.addCell(cellID);
    }
    k = 1 - k;
  }

  for (size_t i = 0; i < Breadth; i++) {
    auto cellID = makeCell(OUT, cells[k][i]);
    netBuilder.addCell(cellID);
  }

  const auto &net = Net::get(netBuilder.make());
  std::cout << net << std::endl;
}

} // namespace eda::gate::model
