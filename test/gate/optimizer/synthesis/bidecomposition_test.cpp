//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/synthesis/bidecomposition.h"
#include "gate/model2/utils/subnet_checking.h"
#include "gate/model2/utils/subnet_truth_table.h"

#include "gtest/gtest.h"
#include "kitty/kitty.hpp"

#include <random>
#include <string>

using BiDecSynthesizer = eda::gate::optimizer::synthesis::BiDecSynthesizer;
using KittyTT          = BiDecSynthesizer::KittyTT;
using Subnet           = eda::gate::model::Subnet;
using TernaryBiClique  = eda::gate::optimizer::synthesis::TernaryBiClique;

bool BiDecompositionTest(size_t numVars) {

  BiDecSynthesizer biDecomposition;

  bool flag{true};
  const size_t funcCount = 10;
  for (size_t i{0}; i < funcCount; ++i) {
    std::string funcString;
    for (int i = 0; i < (1 << numVars); ++i) {
      funcString += std::to_string(std::rand() & 1);
    }
    BiDecSynthesizer::KittyTT tt(numVars);
    kitty::create_from_binary_string(tt, funcString);
    const auto &subnet = Subnet::get(biDecomposition.synthesize(tt));
    flag &= eda::gate::model::utils::equalTruthTables(subnet, tt);
  }

  return flag;
}

TEST(BiDecompositionTest, RandomFunc1Vars) {
  EXPECT_TRUE(BiDecompositionTest(1));
}

TEST(BiDecompositionTest, RandomFunc2Vars) {
  EXPECT_TRUE(BiDecompositionTest(2));
}

TEST(BiDecompositionTest, RandomFunc3Vars) {
  EXPECT_TRUE(BiDecompositionTest(3));
}

TEST(BiDecompositionTest, RandomFunc4Vars) {
  EXPECT_TRUE(BiDecompositionTest(4));
}

TEST(BiDecompositionTest, RandomFunc5Vars) {
  EXPECT_TRUE(BiDecompositionTest(5));
}

TEST(BiDecompositionTest, RandomFunc6Vars) {
  EXPECT_TRUE(BiDecompositionTest(6));
}

TEST(BiDecompositionTest, RandomFunc7Vars) {
  EXPECT_TRUE(BiDecompositionTest(7));
}

TEST(BiDecompositionTest, RandomFunc8Vars) {
  EXPECT_TRUE(BiDecompositionTest(8));
}

TEST(BiDecompositionTest, RandomFunc9Vars) {
  EXPECT_TRUE(BiDecompositionTest(9));
}

TEST(BiDecompositionTest, RandomFunc10Vars) {
  EXPECT_TRUE(BiDecompositionTest(10));
}
