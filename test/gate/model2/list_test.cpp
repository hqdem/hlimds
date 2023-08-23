//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021-2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/cell.h"
#include "gate/model2/list.h"

#include "gtest/gtest.h"

#include <iostream>

namespace eda::gate::model {

TEST(ListTest, SimpleTest) {
  List<Cell> list;

  for (uint64_t i = 1; i <= 1024; i++) {
    list.push_back(i);
  }

  for (auto i = list.begin(); i != list.end(); i++) {
    std::cout << "Item: " << *i << std::endl;
  }
}

} // namespace eda::gate::model
