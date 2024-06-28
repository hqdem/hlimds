//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/utils/subnet_checking.h"
#include "gate/model/utils/subnet_truth_table.h"
#include "gate/optimizer/synthesis/de_micheli.h"

#include "gtest/gtest.h"

#include <string>

using DMSynthesizer = eda::gate::optimizer::synthesis::DMSynthesizer;
using Subnet        = eda::gate::model::Subnet;
using TruthTable    = kitty::dynamic_truth_table;

void runDeMicheliTest(const TruthTable &func, size_t bound = -1) {
  DMSynthesizer alg;
  const auto &subnetId = alg.synthesize(func).id();

  bool invalid = subnetId == eda::gate::model::OBJ_NULL_ID;
  if (invalid) {
    return;
  }

  const auto &subnet = Subnet::get(subnetId);

  bool areEqual = eda::gate::model::utils::equalTruthTables(subnet, func);
  size_t subnetSize = subnet.size();

  EXPECT_TRUE(areEqual && subnetSize <= bound);
}

void runDeMicheliRandomTest(size_t nVars) {
  TruthTable func(nVars);
  kitty::create_random(func);
  runDeMicheliTest(func);
}

//===----------------------------------------------------------------------===//
// Simple Tests
//===----------------------------------------------------------------------===//

TEST(DeMicheliTest, NOT1) {
  // Gate NOT(x).
  TruthTable func(1);
  kitty::create_from_binary_string(func, "01");
  runDeMicheliTest(func, 2);
}

TEST(DeMicheliTest, EQUAL3) {
  TruthTable func(3);
  kitty::create_from_binary_string(func, "11110000");
  runDeMicheliTest(func);
}

TEST(DeMicheliTest, One3) {
  // Gate One().
  TruthTable func(3);
  kitty::create_from_binary_string(func, "11111111");
  runDeMicheliTest(func, 5);
}

TEST(DeMicheliTest, Zero3) {
  // Gate Zero().
  TruthTable func(3);
  kitty::create_from_binary_string(func, "00000000");
  runDeMicheliTest(func, 5);
}

TEST(DeMicheliTest, Xor3) {
  // Gate XOR(x, y, z).
  TruthTable func(3);
  kitty::create_from_binary_string(func, "10010110");
  runDeMicheliTest(func);
}

//===----------------------------------------------------------------------===//
// Majority Gate Test
//===----------------------------------------------------------------------===//

TEST(DeMicheliTest, MAJ3) {
  // Gate MAJ(x, y, z).
  TruthTable func(3);
  kitty::create_from_binary_string(func, "11101000");
  runDeMicheliTest(func, 5);
}

//===----------------------------------------------------------------------===//
// Random Tests
//===----------------------------------------------------------------------===//

TEST(DeMicheliTest, RandomFunc3) {
  // Random gate RAND(x, y, z).
  runDeMicheliRandomTest(3);
}

TEST(DeMicheliTest, RandomFunc4) {
  // Random gate RAND(x, y, z, u).
  runDeMicheliRandomTest(4);
}

TEST(DeMicheliTest, RandomFunc5) {
  // Random gate RAND(x, y, z, u, v).
  runDeMicheliRandomTest(5);
}

TEST(DeMicheliTest, RandomFunc6) {
  // Random gate RAND(x, y, z, u, v, l).
  runDeMicheliRandomTest(6);
}
