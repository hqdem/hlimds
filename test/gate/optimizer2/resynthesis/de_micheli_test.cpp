//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/utils/subnet_truth_table.h"
#include "gate/optimizer2/resynthesis/de_micheli.h"

#include "gtest/gtest.h"

#include <string>

using DeMicheli  = eda::gate::optimizer2::resynthesis::DeMicheli;
using Subnet     = eda::gate::model::Subnet;
using TruthTable = kitty::dynamic_truth_table;

void launchDeMicheliTest(const TruthTable &func, size_t bound = -1) {
  DeMicheli alg;
  const auto &subnetId = alg.synthesize(func);

  bool invalid = subnetId == eda::gate::model::OBJ_NULL_ID;
  if (invalid) {
    return;
  }

  const auto &subnet = Subnet::get(subnetId);

  bool areEqual = (func == eda::gate::model::evaluate(subnet));
  size_t subnetSize = subnet.size();

  EXPECT_TRUE(areEqual && subnetSize <= bound);
}

void launchDeMicheliRandomTest(size_t nVars) {
  TruthTable func(nVars);
  kitty::create_random(func);
  launchDeMicheliTest(func);
}

//===----------------------------------------------------------------------===//
// Simple Tests
//===----------------------------------------------------------------------===//

TEST(DeMicheliTest, NOT1) {
  // Gate NOT(x).
  TruthTable func(1);
  kitty::create_from_binary_string(func, "01");
  launchDeMicheliTest(func, 2);
}

TEST(DeMicheliTest, EQUAL3) {
  TruthTable func(3);
  kitty::create_from_binary_string(func, "11110000");
  launchDeMicheliTest(func);
}

TEST(DeMicheliTest, One3) {
  // Gate One().
  TruthTable func(3);
  kitty::create_from_binary_string(func, "11111111");
  launchDeMicheliTest(func, 5);
}

TEST(DeMicheliTest, Zero3) {
  // Gate Zero().
  TruthTable func(3);
  kitty::create_from_binary_string(func, "00000000");
  launchDeMicheliTest(func, 5);
}

TEST(DeMicheliTest, Xor3) {
  // Gate XOR(x, y, z).
  TruthTable func(3);
  kitty::create_from_binary_string(func, "10010110");
  launchDeMicheliTest(func);
}

//===----------------------------------------------------------------------===//
// Majority Gate Test
//===----------------------------------------------------------------------===//

TEST(DeMicheliTest, MAJ3) {
  // Gate MAJ(x, y, z).
  TruthTable func(3);
  kitty::create_from_binary_string(func, "11101000");
  launchDeMicheliTest(func, 5);
}

//===----------------------------------------------------------------------===//
// Random Tests
//===----------------------------------------------------------------------===//

TEST(DeMicheliTest, RandomFunc3) {
  // Random gate RAND(x, y, z).
  launchDeMicheliRandomTest(3);
}

TEST(DeMicheliTest, RandomFunc4) {
  // Random gate RAND(x, y, z, u).
  launchDeMicheliRandomTest(4);
}

TEST(DeMicheliTest, RandomFunc5) {
  // Random gate RAND(x, y, z, u, v).
  launchDeMicheliRandomTest(5);
}

TEST(DeMicheliTest, RandomFunc6) {
  // Random gate RAND(x, y, z, u, v, l).
  launchDeMicheliRandomTest(6);
}
