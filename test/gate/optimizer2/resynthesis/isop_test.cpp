//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer2/resynthesis/isop.h"
#include "gate/model2/utils/subnet_truth_table.h"

#include "gtest/gtest.h"
#include "kitty/kitty.hpp"

#include <string>

using MinatoMorrealeAlg = eda::gate::optimizer2::resynthesis::MinatoMorrealeAlg;
using Subnet      = eda::gate::model::Subnet;

bool ISOPManagerTest(size_t numVars) {

  MinatoMorrealeAlg minatoMorrealeAlg;

  bool flag = true;
  static const size_t funcCount = 10;
  for (size_t i { 0 }; i < funcCount; ++i) {
    std::string funcString;
    for (int i = 0; i < (1 << numVars); ++i) {
      funcString += std::to_string(std::rand() & 1);
    }
    MinatoMorrealeAlg::KittyTT tt(numVars);
    kitty::create_from_binary_string(tt, funcString);
    const auto &subnet = Subnet::get(minatoMorrealeAlg.synthesize(tt, -1));
    auto ttCopy = eda::gate::model::evaluate(subnet);
    flag &= (ttCopy == tt);
  }

  return flag;
}

TEST(ISOPManagerTest, RandomFunc1Vars) {
  EXPECT_TRUE(ISOPManagerTest(1));
}

TEST(ISOPManagerTest, RandomFunc2Vars) {
  EXPECT_TRUE(ISOPManagerTest(2));
}

TEST(ISOPManagerTest, RandomFunc3Vars) {
  EXPECT_TRUE(ISOPManagerTest(3));
}

TEST(ISOPManagerTest, RandomFunc4Vars) {
  EXPECT_TRUE(ISOPManagerTest(4));
}

TEST(ISOPManagerTest, RandomFunc5Vars) {
  EXPECT_TRUE(ISOPManagerTest(5));
}

TEST(ISOPManagerTest, RandomFunc6Vars) {
  EXPECT_TRUE(ISOPManagerTest(6));
}

TEST(ISOPManagerTest, RandomFunc7Vars) {
  EXPECT_TRUE(ISOPManagerTest(7));
}

TEST(ISOPManagerTest, RandomFunc8Vars) {
  EXPECT_TRUE(ISOPManagerTest(8));
}

TEST(ISOPManagerTest, RandomFunc9Vars) {
  EXPECT_TRUE(ISOPManagerTest(9));
}

TEST(ISOPManagerTest, RandomFunc10Vars) {
  EXPECT_TRUE(ISOPManagerTest(10));
}
