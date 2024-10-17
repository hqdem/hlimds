//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/synthesis/bidecomposition.h"
#include "gate/model/utils/subnet_checking.h"
#include "gate/model/utils/subnet_truth_table.h"

#include "gtest/gtest.h"
#include "kitty/kitty.hpp"

#include <random>
#include <string>

using BiDecSynthesizer = eda::gate::optimizer::synthesis::BiDecSynthesizer;
using Subnet           = eda::gate::model::Subnet;
using TernaryBiClique  = eda::gate::optimizer::synthesis::TernaryBiClique;

void BiDecompositionTest(size_t numVars) {
  BiDecSynthesizer biDecomposition;
  const size_t funcCount = 10;
  for (size_t i{0}; i < funcCount; ++i) {
    std::string funcString;
    for (int i = 0; i < (1 << numVars); ++i) {
      funcString += std::to_string(std::rand() & 1);
    }
    eda::gate::model::TruthTable tt(numVars);
    kitty::create_from_binary_string(tt, funcString);
    const auto &subnet = biDecomposition.synthesize(tt).makeObject();
    EXPECT_EQ(tt, eda::gate::model::evaluate(subnet)[0]);
    const auto &subnetArity = biDecomposition.synthesize(tt, 2).makeObject();
    EXPECT_EQ(tt, eda::gate::model::evaluate(subnetArity)[0]);
  }
}

TEST(BiDecompositionTest, RandomFunc1Vars) {
  BiDecompositionTest(1);
}

TEST(BiDecompositionTest, RandomFunc2Vars) {
  BiDecompositionTest(2);
}

TEST(BiDecompositionTest, RandomFunc3Vars) {
  BiDecompositionTest(3);
}

TEST(BiDecompositionTest, RandomFunc4Vars) {
  BiDecompositionTest(4);
}

TEST(BiDecompositionTest, RandomFunc5Vars) {
  BiDecompositionTest(5);
}

TEST(BiDecompositionTest, RandomFunc6Vars) {
  BiDecompositionTest(6);
}

TEST(BiDecompositionTest, RandomFunc7Vars) {
  BiDecompositionTest(7);
}

TEST(BiDecompositionTest, RandomFunc8Vars) {
  BiDecompositionTest(8);
}

TEST(BiDecompositionTest, RandomFunc9Vars) {
  BiDecompositionTest(9);
}

TEST(BiDecompositionTest, RandomFunc10Vars) {
  BiDecompositionTest(10);
}
