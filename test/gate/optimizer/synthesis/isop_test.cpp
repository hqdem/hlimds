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

void ISOPTest(size_t numVars) {
  MMSynthesizer mm;
  static const size_t funcCount = 100;
  for (size_t i { 0 }; i < funcCount; ++i) {
    std::string funcString;
    for (int i = 0; i < (1 << numVars); ++i) {
      funcString += std::to_string(std::rand() & 1);
    }
    eda::utils::TruthTable tt(numVars);
    kitty::create_from_binary_string(tt, funcString);
    const auto &subnet = mm.synthesize(tt, 2).makeObject();
    EXPECT_EQ(tt, eda::gate::model::evaluate(subnet)[0]);
    const auto &subnetArity = mm.synthesize(tt).makeObject();
    EXPECT_EQ(tt, eda::gate::model::evaluate(subnetArity)[0]);
  }
}

TEST(ISOPTest, RandomFunc1Vars) {
  ISOPTest(1);
}

TEST(ISOPTest, RandomFunc2Vars) {
  ISOPTest(2);
}

TEST(ISOPTest, RandomFunc3Vars) {
  ISOPTest(3);
}

TEST(ISOPTest, RandomFunc4Vars) {
  ISOPTest(4);
}

TEST(ISOPTest, RandomFunc5Vars) {
  ISOPTest(5);
}

TEST(ISOPTest, RandomFunc6Vars) {
  ISOPTest(6);
}

TEST(ISOPTest, RandomFunc7Vars) {
  ISOPTest(7);
}

TEST(ISOPTest, RandomFunc8Vars) {
  ISOPTest(8);
}

TEST(ISOPTest, RandomFunc9Vars) {
  ISOPTest(9);
}

TEST(ISOPTest, RandomFunc10Vars) {
  ISOPTest(10);
}
