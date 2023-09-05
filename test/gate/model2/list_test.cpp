//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021-2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/list.h"

#include "gtest/gtest.h"

#include <vector>

namespace eda::gate::model {

TEST(ListTest, SimpleTest) {
  static constexpr uint64_t A = 1;
  static constexpr uint64_t B = 64*1024;
  static constexpr uint64_t N = (B - A) + 1;

#if 0
  std::vector<uint64_t> list;
  list.reserve(N);
#else
  List<uint64_t> list;
#endif

  for (uint64_t i = A; i <= B; i++) {
    list.push_back(i);
  }

  EXPECT_EQ(list.size(), N);

  uint64_t i = A;
  for (auto iter = list.begin(); iter != list.end(); ++iter) {
    EXPECT_EQ(i, *iter);
    i++;
  }

  i = 0;
  while (!list.empty()) {
    list.erase(list.begin());
    i++;
  }

  EXPECT_EQ(i, N);
}

} // namespace eda::gate::model
