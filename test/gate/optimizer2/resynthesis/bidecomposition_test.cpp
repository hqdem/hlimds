//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer2/resynthesis/bidecomposition.h"

#include "gtest/gtest.h"
#include "kitty/kitty.hpp"

#include <random>
#include <string>

using BiDecompositor = eda::gate::optimizer2::resynthesis::BiDecompositor;
using Subnet         = eda::gate::model::Subnet;
using TruthTable     = BiDecompositor::TruthTable;

bool BiDecompositorTest(const std::string &funcString, size_t numVars) {
  TruthTable func(numVars);
  kitty::create_from_binary_string(func, funcString);

  // Making sure that func has no less then one bit for 1 and no less then
  // one bit for 0.
  if (!kitty::count_ones(func)) {
    kitty::set_bit(func, std::rand() % func.num_bits());
  } else if (!kitty::count_zeros(func)) {
    kitty::clear_bit(func, std::rand() % func.num_bits());
  }

  // Getting a heuristic bi-decomposition for boolean function.
  BiDecompositor biDecompositor;
  const auto &subnet = Subnet::get(biDecompositor.synthesize(func));
  std::cout << subnet << std::endl;

  return true;
}

bool BiDecompositorTest(size_t numVars) {
  std::string funcString;
  for (int j = 0; j < (1 << numVars); ++j) {
    funcString += std::to_string(std::rand() & 1);
  }
  return BiDecompositorTest(funcString, numVars);
}

TEST(BiDecompositorTest, Example5Vars) {
  EXPECT_TRUE(BiDecompositorTest("10110010000000101110001111000000", 5));
}

TEST(BiDecompositorTest, Example3Vars) {
  EXPECT_TRUE(BiDecompositorTest("10010110", 3));
}

TEST(BiDecompositorTest, RandomFunc6Vars) {
  EXPECT_TRUE(BiDecompositorTest(6));
}

TEST(BiDecompositorTest, RandomFunc5Vars) {
  EXPECT_TRUE(BiDecompositorTest(5));
}

TEST(BiDecompositorTest, RandomFunc4Vars) {
  EXPECT_TRUE(BiDecompositorTest(4));
}

TEST(BiDecompositorTest, RandomFunc3Vars) {
  EXPECT_TRUE(BiDecompositorTest(3));
}

TEST(BiDecompositorTest, RandomFunc2Vars) {
  EXPECT_TRUE(BiDecompositorTest(2));
}

TEST(BiDecompositorTest, RandomFunc1Vars) {
  EXPECT_TRUE(BiDecompositorTest(1));
}
