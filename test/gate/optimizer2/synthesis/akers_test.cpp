//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/utils/subnet_checking.h"
#include "gate/model2/utils/subnet_truth_table.h"
#include "gate/optimizer2/synthesis/akers.h"

#include "gtest/gtest.h"
#include "mockturtle/networks/mig.hpp"

#include <ctime>
#include <string>

using AkersAlgorithm = eda::gate::optimizer2::synthesis::AkersAlgorithm;
using KittyTT        = kitty::dynamic_truth_table;
using MIGnetwork     = mockturtle::mig_network;
using Subnet         = eda::gate::model::Subnet;

//===----------------------------------------------------------------------===//
// Convenience Methods
//===----------------------------------------------------------------------===//

void initializeTT2(KittyTT &func, KittyTT &care,
                  std::string bitsFunc, std::string bitsCare) {

  kitty::create_from_binary_string(func, bitsFunc);
  kitty::create_from_binary_string(care, bitsCare);
}

//===----------------------------------------------------------------------===//
// Article Tests
//===----------------------------------------------------------------------===//

/** 
 * The test suite "Akers62" contains examples from the following paper:
 * "Synthesis of combinational logic using three-input majority gates"
 * by Sheldon B. Akers, Jr. (1962).
 */

TEST(Akers62_2, Example1) {
  // Example starts in the page 3
  KittyTT func(3);
  KittyTT care(3);
  initializeTT2(func, care, "11011110", "01101111");

  AkersAlgorithm alg;
  const auto &subnet = alg.synthesize(func, care);

  std::cout << Subnet::get(subnet) << std::endl;

  EXPECT_TRUE(true);
}

TEST(Akers62_2, Example2) {
  // Example starts in the page 4
  KittyTT func(4);
  KittyTT care(4);
  initializeTT2(func, care, "0010001100101010",
                            "1111111111111111");

  AkersAlgorithm alg;
  const auto &subnet = alg.synthesize(func, care);

  std::cout << Subnet::get(subnet) << std::endl;

  EXPECT_TRUE(true);
}

//===----------------------------------------------------------------------===//
// Simple Tests
//===----------------------------------------------------------------------===//

TEST(AkersTest2, NOT1) {
  // Gate NOT(x).
  KittyTT func(1);
  KittyTT care(1);
  initializeTT2(func, care, "01", "11");

  AkersAlgorithm alg;
  const auto subnetId = alg.synthesize(func);
  const auto &subnet = Subnet::get(subnetId);

  bool areEqual = eda::gate::model::utils::equalTruthTables(subnet, func);

  EXPECT_TRUE(areEqual && (subnet.size() == 2));
}

TEST(AkersTest2, One3) {
  // Gate One().
  KittyTT func(3);
  KittyTT care(3);
  initializeTT2(func, care, "11111111", "11111111");

  AkersAlgorithm alg;
  const auto subnetId = alg.synthesize(func);
  const auto &subnet = Subnet::get(subnetId);

  bool areEqual = eda::gate::model::utils::equalTruthTables(subnet, func);

  EXPECT_TRUE(areEqual && (subnet.size() == 5));
}

TEST(AkersTest2, Zero3) {
  // Gate Zero().
  KittyTT func(3);
  KittyTT care(3);
  initializeTT2(func, care, "00000000", "11111111");

  AkersAlgorithm alg;
  const auto subnetId = alg.synthesize(func);
  const auto &subnet = Subnet::get(subnetId);

  bool areEqual = eda::gate::model::utils::equalTruthTables(subnet, func);

  EXPECT_TRUE(areEqual && (subnet.size() == 5));
}

TEST(AkersTest2, OR2) {
  // Gate OR(x, y).
  KittyTT func(2);
  KittyTT care(2);
  initializeTT2(func, care, "1110", "1111");

  AkersAlgorithm alg;
  const auto &subnetId = alg.synthesize(func, care);
  const auto &subnet = Subnet::get(subnetId);

  bool areEqual = eda::gate::model::utils::equalTruthTables(subnet, func);

  EXPECT_TRUE(areEqual && (subnet.size() == 5));
}

TEST(AkersTest2, AND2) {
  // Gate AND(x, y).
  KittyTT func(2);
  KittyTT care(2);
  initializeTT2(func, care, "1000", "1111");

  AkersAlgorithm alg;
  const auto &subnetId = alg.synthesize(func, care);
  const auto &subnet = Subnet::get(subnetId);

  bool areEqual = eda::gate::model::utils::equalTruthTables(subnet, func);

  EXPECT_TRUE(areEqual && (subnet.size() == 5));
}

TEST(AkersTest2, XOR2) {
  // Gate XOR(x, y).
  KittyTT func(2);
  KittyTT care(2);
  initializeTT2(func, care, "0110", "1111");

  AkersAlgorithm alg;
  const auto &subnetId = alg.synthesize(func, care);
  const auto &subnet = Subnet::get(subnetId);

  bool areEqual = eda::gate::model::utils::equalTruthTables(subnet, func);

  EXPECT_TRUE(areEqual && (subnet.size() <= 8));
}

TEST(AkersTest2, XOR3) {
  // Gate XOR(x, y, z).
  KittyTT func(3);
  KittyTT care(3);
  initializeTT2(func, care, "01101001", "11111111");

  AkersAlgorithm alg;
  const auto &subnetId = alg.synthesize(func, care);
  const auto &subnet = Subnet::get(subnetId);

  bool areEqual = eda::gate::model::utils::equalTruthTables(subnet, func);

  EXPECT_TRUE(areEqual && (subnet.size() == 7));
}

TEST(AkersTest2, XOR4) {
  // Gate XOR(x, y, z, v).
  KittyTT func(4);
  KittyTT care(4);
  initializeTT2(func, care, "0110100110010110", "1111111111111111");

  AkersAlgorithm alg;
  const auto &subnetId = alg.synthesize(func, care);
  const auto &subnet = Subnet::get(subnetId);

  bool areEqual = eda::gate::model::utils::equalTruthTables(subnet, func);

  EXPECT_TRUE(areEqual && (subnet.size() <= 18));
}

//===----------------------------------------------------------------------===//
// Majority Gate Tests
//===----------------------------------------------------------------------===//

TEST(AkersTest2, MAJ3) {
  // Gate MAJ(x, y, z).
  KittyTT func(3);
  KittyTT care(3);
  initializeTT2(func, care, "11101000", "11111111");

  AkersAlgorithm alg;
  const auto &subnetId = alg.synthesize(func, care);
  const auto &subnet = Subnet::get(subnetId);

  bool areEqual = eda::gate::model::utils::equalTruthTables(subnet, func);

  EXPECT_TRUE(areEqual && (subnet.size() == 5));
}

TEST(AkersTest2, MAJ5) {
  // Gate MAJ(x, y, z, u, v).
  KittyTT func(5);
  KittyTT care(5);
  initializeTT2(func, care, "11111110111010001110100010000000",
                            "11111111111111111111111111111111");

  AkersAlgorithm alg;
  const auto &subnetId = alg.synthesize(func, care);
  const auto &subnet = Subnet::get(subnetId);

  bool areEqual = eda::gate::model::utils::equalTruthTables(subnet, func);

  EXPECT_TRUE(areEqual && (subnet.size() == 10));
}

//===----------------------------------------------------------------------===//
// Random Tests
//===----------------------------------------------------------------------===//

TEST(AkersTest2, RandomFunc5) {
  // Random gate RAND(x, y, z, u, v).
  KittyTT func(5);
  kitty::create_random(func);

  AkersAlgorithm alg;
  const auto &subnetId = alg.synthesize(func);
  const auto &subnet = Subnet::get(subnetId);

  bool areEqual = eda::gate::model::utils::equalTruthTables(subnet, func);

  EXPECT_TRUE(areEqual);
}

TEST(AkersTest2, RandomFunc6) {
  // Random gate RAND(x, y, z, u, v, w).
  KittyTT func(6);
  kitty::create_random(func);

  AkersAlgorithm alg;
  const auto &subnetId = alg.synthesize(func);
  const auto &subnet = Subnet::get(subnetId);

  bool areEqual = eda::gate::model::utils::equalTruthTables(subnet, func);

  EXPECT_TRUE(areEqual);
}

TEST(AkersTest2, RandomFunc7) {
  // Random gate RAND(x, y, z, u, v, w, p).
  KittyTT func(7);
  kitty::create_random(func);

  AkersAlgorithm alg;
  const auto &subnetId = alg.synthesize(func);
  const auto &subnet = Subnet::get(subnetId);

  bool areEqual = eda::gate::model::utils::equalTruthTables(subnet, func);

  EXPECT_TRUE(areEqual);
}

TEST(AkersTest2, RandomFunc8) {
  // Random gate RAND(x, y, z, u, v, w, p, h).
  KittyTT func(8);
  kitty::create_random(func);

  AkersAlgorithm alg;
  const auto &subnetId = alg.synthesize(func);
  const auto &subnet = Subnet::get(subnetId);

  bool areEqual = eda::gate::model::utils::equalTruthTables(subnet, func);

  EXPECT_TRUE(areEqual);
}
