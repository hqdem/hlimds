//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/utils/subnet_checking.h"
#include "gate/model/utils/subnet_truth_table.h"
#include "gate/optimizer/synthesis/akers.h"

#include "gtest/gtest.h"
#include "mockturtle/networks/mig.hpp"

#include <ctime>
#include <string>

using AkersSynthesizer = eda::gate::optimizer::synthesis::AkersSynthesizer;
using MIGnetwork       = mockturtle::mig_network;
using Subnet           = eda::gate::model::Subnet;
using TruthTable       = kitty::dynamic_truth_table;

//===----------------------------------------------------------------------===//
// Convenience Methods
//===----------------------------------------------------------------------===//

void runAkersSynthesizerTest(const TruthTable &func, size_t maxSize = -1) {
  AkersSynthesizer akers;

  const auto &s = akers.synthesize(func).makeObject();
  bool areEqual = eda::gate::model::utils::equalTruthTables(s, func);

  EXPECT_TRUE(areEqual && (s.size() <= maxSize));
}

void runAkersSynthesizerTest(const std::string &func,
                             size_t nVars,
                             size_t maxSize = -1) {

  TruthTable table(nVars);
  kitty::create_from_binary_string(table, func);
  runAkersSynthesizerTest(table, maxSize);
}

void runAkersSynthesizerTest(size_t nVars) {
  TruthTable table(nVars);
  kitty::create_random(table);
  runAkersSynthesizerTest(table);
}

//===----------------------------------------------------------------------===//
// Article Tests
//===----------------------------------------------------------------------===//

/** 
 * The test suite "Akers62" contains examples from the following paper:
 * "Synthesis of combinational logic using three-input majority gates"
 * by Sheldon B. Akers, Jr. (1962).
 */

TEST(Akers62, Example1) {
  // Example starts in the page 3
  TruthTable func(3);
  TruthTable care(3);

  kitty::create_from_binary_string(func, "11011110");
  kitty::create_from_binary_string(care, "01101111");

  AkersSynthesizer alg;
  const auto &subnet = alg.synthesize(func, care).makeObject();

  std::cout << subnet << std::endl;
}

TEST(Akers62, Example2) {
  // Example starts in the page 4
  TruthTable func(4);
  kitty::create_from_binary_string(func, "0010001100101010");

  AkersSynthesizer alg;
  const auto &subnet = alg.synthesize(func).makeObject();

  std::cout << subnet << std::endl;
}

//===----------------------------------------------------------------------===//
// Simple Tests
//===----------------------------------------------------------------------===//

TEST(AkersTest, NOT1) {
  // Gate NOT(x).
  runAkersSynthesizerTest("01", 1, 2);
}

TEST(AkersTest, One3) {
  // Gate One().
  runAkersSynthesizerTest("11111111", 3, 5);
}

TEST(AkersTest, Zero3) {
  // Gate Zero().
  runAkersSynthesizerTest("00000000", 3, 5);
}

TEST(AkersTest, OR2) {
  // Gate OR(x, y).
  runAkersSynthesizerTest("1110", 2, 5);
}

TEST(AkersTest, AND2) {
  // Gate AND(x, y).
  runAkersSynthesizerTest("1000", 2, 5);
}

TEST(AkersTest, XOR2) {
  // Gate XOR(x, y).
  runAkersSynthesizerTest("0110", 2, 8);
}

TEST(AkersTest, XOR3) {
  // Gate XOR(x, y, z).
  runAkersSynthesizerTest("01101001", 3, 7);
}

TEST(AkersTest, XOR4) {
  // Gate XOR(x, y, z, v).
  runAkersSynthesizerTest("0110100110010110", 4, 18);
}

//===----------------------------------------------------------------------===//
// Majority Gate Tests
//===----------------------------------------------------------------------===//

TEST(AkersTest, MAJ3) {
  // Gate MAJ(x, y, z).
  runAkersSynthesizerTest("11101000", 3, 5);
}

TEST(AkersTest, MAJ5) {
  // Gate MAJ(x, y, z, u, v).
  runAkersSynthesizerTest("11111110111010001110100010000000", 5, 10);
}

//===----------------------------------------------------------------------===//
// Random Tests
//===----------------------------------------------------------------------===//

TEST(AkersTest, RandomFunc5) {
  // Random gate RAND(x, y, z, u, v).
  runAkersSynthesizerTest(5);
}

TEST(AkersTest, RandomFunc6) {
  // Random gate RAND(x, y, z, u, v, w).
  runAkersSynthesizerTest(6);
}

TEST(AkersTest, RandomFunc7) {
  // Random gate RAND(x, y, z, u, v, w, p).
  runAkersSynthesizerTest(7);
}

TEST(AkersTest, RandomFunc8) {
  // Random gate RAND(x, y, z, u, v, w, p, h).
  runAkersSynthesizerTest(8);
}
