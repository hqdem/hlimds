//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gnet.h"
#include "rtl/compiler/compiler.h"
#include "rtl/library/flibrary.h"
#include "rtl/model/net.h"
#include "rtl/parser/ril/builder.h"
#include "rtl/parser/ril/parser.h"

#include "gtest/gtest.h"

#include <filesystem>
#include <iostream>
#include <memory>

using namespace eda::gate::model;
using namespace eda::rtl::compiler;
using namespace eda::rtl::library;
using namespace eda::rtl::model;
using namespace eda::rtl::parser::ril;

namespace fs = std::filesystem;

int rilTest(const std::string &outSubPath,
            const std::string &fileName) {
  fs::path basePath = std::getenv("UTOPIA_HOME");
  fs::path fullPath = basePath / outSubPath / fileName;

  auto model = parse(fullPath);

  std::cout << "------ p/v-nets ------" << std::endl;
  std::cout << *model << std::endl;

  Compiler compiler(FLibraryDefault::get());
  auto gnet = compiler.compile(*model);

  std::cout << "------ g-net ------" << std::endl;
  std::cout << *gnet;

  return 0;
}

// TODO: uncomment when RIL parser will be fixed
// TEST(RilTest, DffTest) {
//   EXPECT_EQ(rilTest("test/data/ril/", "dff.ril"), 0);
// }

TEST(RilTest, SingleTest) {
  EXPECT_EQ(rilTest("test/data/ril", "test.ril"), 0);
}

TEST(RilTest, FuncTest) {
  EXPECT_EQ(rilTest("test/data/ril", "func.ril"), 0);
}
