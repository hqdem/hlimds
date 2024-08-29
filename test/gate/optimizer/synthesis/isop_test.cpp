//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
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

void IsopTest(size_t numVars) {
  MMSynthesizer mm;
  static const size_t funcCount = 100;
  for (size_t i { 0 }; i < funcCount; ++i) {
    std::string funcString;
    for (int i = 0; i < (1 << numVars); ++i) {
      funcString += std::to_string(std::rand() & 1);
    }
    eda::util::TruthTable tt(numVars);
    kitty::create_from_binary_string(tt, funcString);
    const auto &subnet = mm.synthesize(tt, 2).makeObject();
    EXPECT_EQ(tt, eda::gate::model::evaluate(subnet)[0]);
    const auto &subnetArity = mm.synthesize(tt).makeObject();
    EXPECT_EQ(tt, eda::gate::model::evaluate(subnetArity)[0]);
  }
}

TEST(IsopTest, RandomFunc1Vars) {
  IsopTest(1);
}

TEST(IsopTest, RandomFunc2Vars) {
  IsopTest(2);
}

TEST(IsopTest, RandomFunc3Vars) {
  IsopTest(3);
}

TEST(IsopTest, RandomFunc4Vars) {
  IsopTest(4);
}

TEST(IsopTest, RandomFunc5Vars) {
  IsopTest(5);
}

TEST(IsopTest, RandomFunc6Vars) {
  IsopTest(6);
}

TEST(IsopTest, RandomFunc7Vars) {
  IsopTest(7);
}

TEST(IsopTest, RandomFunc8Vars) {
  IsopTest(8);
}

TEST(IsopTest, RandomFunc9Vars) {
  IsopTest(9);
}

TEST(IsopTest, RandomFunc10Vars) {
  IsopTest(10);
}
