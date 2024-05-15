//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/utils/subnet_checking.h"
#include "gate/model/utils/subnet_truth_table.h"
#include "gate/optimizer/synthesis/algebraic_factor.h"

#include "gtest/gtest.h"
#include "kitty/kitty.hpp"

#include <string>

using AlgebraicFactor = eda::gate::optimizer::synthesis::AlgebraicFactor;
using Subnet          = eda::gate::model::Subnet;

bool algebraicFactorTest(size_t numVars) {

  AlgebraicFactor factor;

  bool flag = true;
  static const size_t funcCount = 100;
  for (size_t i { 0 }; i < funcCount; ++i) {
    std::string funcString;
    for (int i = 0; i < (1 << numVars); ++i) {
      funcString += std::to_string(std::rand() & 1);
    }
    kitty::dynamic_truth_table tt(numVars);
    kitty::create_from_binary_string(tt, funcString);
    if (kitty::is_const0(~tt) || kitty::is_const0(tt)) {
      continue;
    }
    const auto &subnet = Subnet::get(
        factor.getSubnet(kitty::isop(tt), numVars, 2));
    flag &= eda::gate::model::utils::equalTruthTables(subnet, tt);
  }

  return flag;
}

TEST(AlgebraicFactorTest, RandomFunc3Vars) {
  EXPECT_TRUE(algebraicFactorTest(3));
}

TEST(AlgebraicFactorTest, RandomFunc4Vars) {
  EXPECT_TRUE(algebraicFactorTest(4));
}

TEST(AlgebraicFactorTest, RandomFunc5Vars) {
  EXPECT_TRUE(algebraicFactorTest(5));
}

TEST(AlgebraicFactorTest, RandomFunc6Vars) {
  EXPECT_TRUE(algebraicFactorTest(6));
}

TEST(AlgebraicFactorTest, RandomFunc7Vars) {
  EXPECT_TRUE(algebraicFactorTest(7));
}

TEST(AlgebraicFactorTest, RandomFunc8Vars) {
  EXPECT_TRUE(algebraicFactorTest(8));
}

TEST(AlgebraicFactorTest, RandomFunc9Vars) {
  EXPECT_TRUE(algebraicFactorTest(9));
}

TEST(AlgebraicFactorTest, RandomFunc10Vars) {
  EXPECT_TRUE(algebraicFactorTest(10));
}
