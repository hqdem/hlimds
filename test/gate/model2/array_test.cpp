//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021-2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/array.h"

#include "gtest/gtest.h"

#include <vector>

namespace eda::gate::model {

TEST(ArrayTest, SimpleTest) {
  static constexpr uint64_t A = 1;
  static constexpr uint64_t B = 64*1024;
  static constexpr uint64_t N = (B - A) + 1;

#if 0
  std::vector<uint64_t> array(N);
#else
  Array<uint64_t> array(N);
#endif

  EXPECT_EQ(array.size(), N);

  uint64_t v = A;
  for (uint64_t i = 0; i <= N; i++) {
    array[i] = v++;
  }

  uint64_t w = A;
  for (auto iter = array.begin(); iter != array.end(); ++iter) {
    EXPECT_EQ(w, *iter);
    w++;
  }
}

} // namespace eda::gate::model
