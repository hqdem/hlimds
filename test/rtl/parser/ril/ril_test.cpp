//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "rtl/model/net.h"
#include "rtl/parser/ril/builder.h"
#include "rtl/parser/ril/parser.h"
#include "util/env.h"
#include "util/logging.h"

#include "gtest/gtest.h"

#include <filesystem>
#include <iostream>
#include <memory>

using namespace eda::rtl::model;
using namespace eda::rtl::parser::ril;

int rilTest(const std::string &outSubPath,
            const std::string &fileName) {
  std::filesystem::path basePath = eda::env::getHomePath();
  std::filesystem::path fullPath = basePath / outSubPath / fileName;

  auto model = parse(fullPath);

  LOG_DEBUG("------ p/v-nets ------");
  LOG_DEBUG(*model);

  return 0;
}

#if 0
TEST(RilTest, DffTest) {
  EXPECT_EQ(rilTest("test/data/ril/", "dff.ril"), 0);
}
#endif

TEST(RilTest, SingleTest) {
  EXPECT_EQ(rilTest("test/data/ril", "test.ril"), 0);
}

TEST(RilTest, FuncTest) {
  EXPECT_EQ(rilTest("test/data/ril", "func.ril"), 0);
}
