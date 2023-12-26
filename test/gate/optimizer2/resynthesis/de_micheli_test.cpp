//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer2/resynthesis/de_micheli.h"
#include "gate/model2/utils/subnet_truth_table.h"

#include "gtest/gtest.h"
#include "mockturtle/networks/mig.hpp"

#include <ctime>
#include <string>

using DeMicheli  = eda::gate::optimizer2::resynthesis::DeMicheli;
using TruthTable = kitty::dynamic_truth_table;
using Subnet     = eda::gate::model::Subnet;

//===----------------------------------------------------------------------===//
// Simple Tests
//===----------------------------------------------------------------------===//

TEST(DeMicheliTest, NOT1) {
  // Gate NOT(x).
  TruthTable func(1);
  kitty::create_from_binary_string(func, "01");

  DeMicheli alg;
  const auto subnetId = alg.synthesize(func);
  const auto &subnet = Subnet::get(subnetId);

  bool areEqual = (func == eda::gate::model::evaluate(subnet));

  EXPECT_TRUE(areEqual && (subnet.size() == 2));
}

TEST(DeMicheliTest, EQUAL3) {
  TruthTable func(3);
  kitty::create_from_binary_string(func, "11110000");

  DeMicheli alg;
  const auto subnetId = alg.synthesize(func);
  const auto &subnet = Subnet::get(subnetId);

  bool areEqual = (func == eda::gate::model::evaluate(subnet));

  EXPECT_TRUE(areEqual);
}

TEST(DeMicheliTest, One3) {
  // Gate One().
  TruthTable func(3);
  kitty::create_from_binary_string(func, "11111111");

  DeMicheli alg;
  const auto subnetId = alg.synthesize(func);
  const auto &subnet = Subnet::get(subnetId);

  bool areEqual = (func == eda::gate::model::evaluate(subnet));

  EXPECT_TRUE(areEqual && (subnet.size() == 5));
}

TEST(DeMicheliTest, Zero3) {
  // Gate Zero().
  TruthTable func(3);
  kitty::create_from_binary_string(func, "00000000");

  DeMicheli alg;
  const auto subnetId = alg.synthesize(func);
  const auto &subnet = Subnet::get(subnetId);

  bool areEqual = (func == eda::gate::model::evaluate(subnet));

  EXPECT_TRUE(areEqual && (subnet.size() == 5));
}

TEST(DeMicheliTest, Xor3) {
  // Gate XOR(x, y, z).
  TruthTable func(3);
  kitty::create_from_binary_string(func, "10010110");

  DeMicheli alg;
  const auto subnetId = alg.synthesize(func);
  const auto &subnet = Subnet::get(subnetId);

  bool areEqual = (func == eda::gate::model::evaluate(subnet));

  EXPECT_TRUE(areEqual);
}

//===----------------------------------------------------------------------===//
// Majority Gate Test
//===----------------------------------------------------------------------===//

TEST(DeMicheliTest, MAJ3) {
  // Gate MAJ(x, y, z).
  TruthTable func(3);
  kitty::create_from_binary_string(func, "11101000");

  DeMicheli alg;
  const auto &subnetId = alg.synthesize(func);
  const auto &subnet = Subnet::get(subnetId);

  bool areEqual = (func == eda::gate::model::evaluate(subnet));

  EXPECT_TRUE(areEqual && (subnet.size() == 5));
}

//===----------------------------------------------------------------------===//
// Random Tests
//===----------------------------------------------------------------------===//

TEST(DeMicheliTest, RandomFunc3) {
  // Random gate RAND(x, y, z).
  TruthTable func(3);
  kitty::create_random(func);

  DeMicheli alg;
  const auto &subnetId = alg.synthesize(func);
  bool areEqual = false;
  if (subnetId != eda::gate::model::OBJ_NULL_ID) {
    const auto &subnet = Subnet::get(subnetId);
    areEqual = (func == eda::gate::model::evaluate(subnet));
  } else {
    areEqual = true;
    std::cout << "NOT FOUND\n";
  }

  EXPECT_TRUE(areEqual);
}

TEST(DeMicheliTest, RandomFunc4) {
  // Random gate RAND(x, y, z, u).
  TruthTable func(4);
  kitty::create_random(func);

  DeMicheli alg;
  const auto &subnetId = alg.synthesize(func);
  bool areEqual = false;
  if (subnetId != eda::gate::model::OBJ_NULL_ID) {
    const auto &subnet = Subnet::get(subnetId);
    areEqual = (func == eda::gate::model::evaluate(subnet));
  } else {
    areEqual = true;
    std::cout << "NOT FOUND\n";
  }

  EXPECT_TRUE(areEqual);
}

TEST(DeMicheliTest, RandomFunc5) {
  // Random gate RAND(x, y, z, u, v).
  TruthTable func(5);
  kitty::create_random(func);

  DeMicheli alg;
  const auto &subnetId = alg.synthesize(func);
  bool areEqual = false;
  if (subnetId != eda::gate::model::OBJ_NULL_ID) {
    const auto &subnet = Subnet::get(subnetId);
    areEqual = (func == eda::gate::model::evaluate(subnet));
  } else {
    areEqual = true;
    std::cout << "NOT FOUND\n";
  }

  EXPECT_TRUE(areEqual);
}

TEST(DeMicheliTest, RandomFunc6) {
  // Random gate RAND(x, y, z, u, v).
  TruthTable func(6);
  kitty::create_random(func);

  DeMicheli alg;
  const auto &subnetId = alg.synthesize(func);
  bool areEqual = false;
  if (subnetId != eda::gate::model::OBJ_NULL_ID) {
    const auto &subnet = Subnet::get(subnetId);
    areEqual = (func == eda::gate::model::evaluate(subnet));
  } else {
    areEqual = true;
    std::cout << "NOT FOUND\n";
  }

  EXPECT_TRUE(areEqual);
}
