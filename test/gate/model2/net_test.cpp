//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021-2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/net.h"

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
    cells[0][i] = makeCell(IN);
  }

  size_t k = 0;
  for (size_t i = 0; i < Depth; i++) {
    for (size_t j = 0; j < Breadth; j++) {
      cells[1 - k][j] = makeCell(AND, cells[k][j], cells[k][(Breadth - j) - 1]);
    }
    k = 1 - k;
  }

  for (size_t i = 0; i < Breadth; i++) {
    makeCell(OUT, cells[k][i]);
  }
}

} // namespace eda::gate::model
