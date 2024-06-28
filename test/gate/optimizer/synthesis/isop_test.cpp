//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/synthesis/isop.h"
#include "gate/model/utils/subnet_checking.h"
#include "gate/model/utils/subnet_truth_table.h"

#include "gtest/gtest.h"
#include "kitty/kitty.hpp"

#include <string>

using MMSynthesizer = eda::gate::optimizer::synthesis::MMSynthesizer;
using Subnet        = eda::gate::model::Subnet;

bool ISOPTest(size_t numVars) {

  MMSynthesizer minatoMorrealeAlg;

  bool flag = true;
  static const size_t funcCount = 100;
  for (size_t i { 0 }; i < funcCount; ++i) {
    std::string funcString;
    for (int i = 0; i < (1 << numVars); ++i) {
      funcString += std::to_string(std::rand() & 1);
    }
    eda::utils::TruthTable tt(numVars);
    kitty::create_from_binary_string(tt, funcString);
    const auto &subnet = minatoMorrealeAlg.synthesize(tt, 3).object();
    flag &= eda::gate::model::utils::equalTruthTables(subnet, tt);
  }

  return flag;
}

TEST(ISOPTest, RandomFunc1Vars) {
  EXPECT_TRUE(ISOPTest(1));
}

TEST(ISOPTest, RandomFunc2Vars) {
  EXPECT_TRUE(ISOPTest(2));
}

TEST(ISOPTest, RandomFunc3Vars) {
  EXPECT_TRUE(ISOPTest(3));
}

TEST(ISOPTest, RandomFunc4Vars) {
  EXPECT_TRUE(ISOPTest(4));
}

TEST(ISOPTest, RandomFunc5Vars) {
  EXPECT_TRUE(ISOPTest(5));
}

TEST(ISOPTest, RandomFunc6Vars) {
  EXPECT_TRUE(ISOPTest(6));
}

TEST(ISOPTest, RandomFunc7Vars) {
  EXPECT_TRUE(ISOPTest(7));
}

TEST(ISOPTest, RandomFunc8Vars) {
  EXPECT_TRUE(ISOPTest(8));
}

TEST(ISOPTest, RandomFunc9Vars) {
  EXPECT_TRUE(ISOPTest(9));
}

TEST(ISOPTest, RandomFunc10Vars) {
  EXPECT_TRUE(ISOPTest(10));
}
