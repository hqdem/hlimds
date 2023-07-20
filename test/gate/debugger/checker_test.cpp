//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "lec_test.h"

using namespace eda::gate::debugger;
using namespace eda::gate::model;

TEST(CheckGNetTest, CheckNorNorSmallTest) {
  EXPECT_TRUE(checkNorNorTest(8));
}

TEST(CheckGNetTest, CheckNorAndnSmallTest) {
  EXPECT_TRUE(checkNorAndnTest(8));
}

TEST(CheckGNetTest, CheckNorAndSmallTest) {
  EXPECT_FALSE(checkNorAndTest(8));
}

TEST(CheckGNetTest, CheckNorNorTest) {
  EXPECT_TRUE(checkNorNorTest(256));
}

TEST(CheckGNetTest, CheckNorAndnTest) {
  EXPECT_TRUE(checkNorAndnTest(256));
}

TEST(CheckGNetTest, CheckNorAndTest) {
  EXPECT_FALSE(checkNorAndTest(256));
}
