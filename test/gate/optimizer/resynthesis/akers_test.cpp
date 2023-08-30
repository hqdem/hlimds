//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/resynthesis/akers.h"
#include "gate/optimizer/rwdatabase.h"
#include "gate/optimizer/truthtable.h"

#include "gtest/gtest.h"

#include <kitty/constructors.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/algorithms/akers_synthesis.hpp>

#include <random>
#include <string>
#include <time.h>

using AkersAlgorithm = eda::gate::optimizer::resynthesis::AkersAlgorithm;
using BGNet = eda::gate::optimizer::BoundGNet;
using Gate = eda::gate::model::Gate;
using KittyTT = kitty::dynamic_truth_table;
using MIGnetwork = mockturtle::mig_network;
using SignalList = Gate::SignalList;
using TruthTable = eda::gate::optimizer::TruthTable;

//===----------------------------------------------------------------------===//
// Convenience Methods
//===----------------------------------------------------------------------===//

void initializeTT(KittyTT &func, KittyTT &care,
                  std::string bitsFunc, std::string bitsCare) {

  kitty::create_from_binary_string(func, bitsFunc);
  kitty::create_from_binary_string(care, bitsCare);
}

bool areEqualTT(KittyTT func, KittyTT care, TruthTable tt) {
  for (size_t pos = 0; pos < func.num_bits(); pos++) {
    if (kitty::get_bit(care, pos)) {
      bool bit = (((tt.raw() >> pos) & 1) != 0);
      if (kitty::get_bit(func, pos) != bit) {
        return false;
      }
    }
  }
  return true;
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
  KittyTT func(3);
  KittyTT care(3);
  initializeTT(func, care, "11011110", "01101111");

  Gate::SignalList inputs;
  Gate::Id outputId;
  BGNet bGNet;

  AkersAlgorithm alg(func, care);
  bGNet.net = alg.run(inputs, outputId);
  bGNet.inputBindings = {inputs[0].node(), inputs[1].node(), inputs[2].node()};
  bGNet.outputBindings = {outputId};

  TruthTable gNetTT = TruthTable::build(bGNet);

  bool equal = areEqualTT(func, care, gNetTT);

  std::cout << *bGNet.net;

  EXPECT_TRUE(equal);
}

TEST(Akers62, Example2) {
  // Example starts in the page 4
  KittyTT func(4);
  KittyTT care(4);
  initializeTT(func, care, "0010001100101010",
                           "1111111111111111");

  Gate::SignalList inputs;
  Gate::Id outputId;
  BGNet bGNet;

  AkersAlgorithm alg(func, care);
  bGNet.net = alg.run(inputs, outputId);
  bGNet.inputBindings = {inputs[0].node(), inputs[1].node(),
                         inputs[2].node(), inputs[3].node()};
  bGNet.outputBindings = {outputId};

  TruthTable gNetTT = TruthTable::build(bGNet);

  bool equal = areEqualTT(func, care, gNetTT);

  std::cout << *bGNet.net;

  EXPECT_TRUE(equal);
}

//===----------------------------------------------------------------------===//
// Simple Tests
//===----------------------------------------------------------------------===//

TEST(AkersTest, NOT1) {
  // Gate NOT(x).
  KittyTT func(1);
  KittyTT care(1);
  initializeTT(func, care, "01", "11");

  Gate::SignalList inputs;
  Gate::Id outputId;
  BGNet bGNet;

  AkersAlgorithm alg(func, care);
  bGNet.net = alg.run(inputs, outputId);
  bGNet.inputBindings = {inputs[0].node()};
  bGNet.outputBindings = {outputId};

  TruthTable gNetTT = TruthTable::build(bGNet);

  bool equal = areEqualTT(func, care, gNetTT);

  EXPECT_TRUE(equal && (alg.nMaj == 0));
}

TEST(AkersTest, One3) {
  // Gate One().
  KittyTT func(3);
  KittyTT care(3);
  initializeTT(func, care, "11111111", "11111111");

  Gate::SignalList inputs;
  Gate::Id outputId;
  BGNet bGNet;

  AkersAlgorithm alg(func, care);
  bGNet.net = alg.run(inputs, outputId);
  bGNet.inputBindings = {inputs[0].node(), inputs[1].node(), inputs[2].node()};
  bGNet.outputBindings = {outputId};

  TruthTable gNetTT = TruthTable::build(bGNet);

  bool equal = areEqualTT(func, care, gNetTT);

  EXPECT_TRUE(equal && (alg.nMaj == 0));
}

TEST(AkersTest, Zero3) {
  // Gate Zero().
  KittyTT func(3);
  KittyTT care(3);
  initializeTT(func, care, "00000000", "11111111");

  Gate::SignalList inputs;
  Gate::Id outputId;
  BGNet bGNet;

  AkersAlgorithm alg(func, care);
  bGNet.net = alg.run(inputs, outputId);
  bGNet.inputBindings = {inputs[0].node(), inputs[1].node(), inputs[2].node()};
  bGNet.outputBindings = {outputId};

  TruthTable gNetTT = TruthTable::build(bGNet);

  bool equal = areEqualTT(func, care, gNetTT);

  EXPECT_TRUE(equal && (alg.nMaj == 0));
}

TEST(AkersTest, OR2) {
  // Gate OR(x, y).
  KittyTT func(2);
  KittyTT care(2);
  initializeTT(func, care, "1110", "1111");

  Gate::SignalList inputs;
  Gate::Id outputId;
  BGNet bGNet;

  AkersAlgorithm alg(func, care);
  bGNet.net = alg.run(inputs, outputId);
  bGNet.inputBindings = {inputs[0].node(), inputs[1].node()};
  bGNet.outputBindings = {outputId};

  TruthTable gNetTT = TruthTable::build(bGNet);

  bool equal = areEqualTT(func, care, gNetTT);

  EXPECT_TRUE(equal && (alg.nMaj == 1));
}

TEST(AkersTest, AND2) {
  // Gate AND(x, y).
  KittyTT func(2);
  KittyTT care(2);
  initializeTT(func, care, "1000", "1111");

  Gate::SignalList inputs;
  Gate::Id outputId;
  BGNet bGNet;

  AkersAlgorithm alg(func, care);
  bGNet.net = alg.run(inputs, outputId);
  bGNet.inputBindings = {inputs[0].node(), inputs[1].node()};
  bGNet.outputBindings = {outputId};

  TruthTable gNetTT = TruthTable::build(bGNet);

  bool equal = areEqualTT(func, care, gNetTT);

  EXPECT_TRUE(equal && (alg.nMaj == 1));
}

TEST(AkersTest, XOR2) {
  // Gate XOR(x, y).
  KittyTT func(2);
  KittyTT care(2);
  initializeTT(func, care, "0110", "1111");

  Gate::SignalList inputs;
  Gate::Id outputId;
  BGNet bGNet;

  AkersAlgorithm alg(func, care);
  bGNet.net = alg.run(inputs, outputId);
  bGNet.inputBindings = {inputs[0].node(), inputs[1].node()};
  bGNet.outputBindings = {outputId};

  TruthTable gNetTT = TruthTable::build(bGNet);

  bool equal = areEqualTT(func, care, gNetTT);

  EXPECT_TRUE(equal && (alg.nMaj == 3));
}

TEST(AkersTest, XOR3) {
  // Gate XOR(x, y, z).
  KittyTT func(3);
  KittyTT care(3);
  initializeTT(func, care, "01101001", "11111111");

  Gate::SignalList inputs;
  Gate::Id outputId;
  BGNet bGNet;

  AkersAlgorithm alg(func, care);
  bGNet.net = alg.run(inputs, outputId);
  bGNet.inputBindings = {inputs[0].node(), inputs[1].node(), inputs[2].node()};
  bGNet.outputBindings = {outputId};

  TruthTable gNetTT = TruthTable::build(bGNet);

  bool equal = areEqualTT(func, care, gNetTT);

  EXPECT_TRUE(equal && (alg.nMaj == 3));
}

TEST(AkersTest, XOR4) {
  // Gate XOR(x, y, z, v).
  KittyTT func(4);
  KittyTT care(4);
  initializeTT(func, care, "0110100110010110", "1111111111111111");

  Gate::SignalList inputs;
  Gate::Id outputId;
  BGNet bGNet;

  AkersAlgorithm alg(func, care);
  bGNet.net = alg.run(inputs, outputId);
  bGNet.inputBindings = {inputs[0].node(), inputs[1].node(),
                         inputs[2].node(), inputs[3].node()};
  bGNet.outputBindings = {outputId};

  TruthTable gNetTT = TruthTable::build(bGNet);

  bool equal = areEqualTT(func, care, gNetTT);

  EXPECT_TRUE(equal && (alg.nMaj <= 11));
}

//===----------------------------------------------------------------------===//
// Majority Gate Tests
//===----------------------------------------------------------------------===//

TEST(AkersTest, MAJ3) {
  // Gate MAJ(x, y, z).
  KittyTT func(3);
  KittyTT care(3);
  initializeTT(func, care, "11101000", "11111111");

  Gate::SignalList inputs;
  Gate::Id outputId;
  BGNet bGNet;

  AkersAlgorithm alg(func, care);
  bGNet.net = alg.run(inputs, outputId);
  bGNet.inputBindings = {inputs[0].node(), inputs[1].node(), inputs[2].node()};
  bGNet.outputBindings = {outputId};

  TruthTable gNetTT = TruthTable::build(bGNet);

  bool equal = areEqualTT(func, care, gNetTT);

  EXPECT_TRUE(equal && (alg.nMaj == 1));
}

TEST(AkersTest, MAJ5) {
  // Gate MAJ(x, y, z, u, v).
  KittyTT func(5);
  KittyTT care(5);
  initializeTT(func, care, "11111110111010001110100010000000",
                           "11111111111111111111111111111111");

  Gate::SignalList inputs;
  Gate::Id outputId;
  BGNet bGNet;

  AkersAlgorithm alg(func, care);
  bGNet.net = alg.run(inputs, outputId);
  bGNet.inputBindings = {inputs[0].node(), inputs[1].node(), inputs[2].node(),
                         inputs[3].node(), inputs[4].node()};
  bGNet.outputBindings = {outputId};

  TruthTable gNetTT = TruthTable::build(bGNet);

  bool equal = areEqualTT(func, care, gNetTT);

  EXPECT_TRUE(equal && (alg.nMaj == 4));
}

//===----------------------------------------------------------------------===//
// Random Tests
//===----------------------------------------------------------------------===//

TEST(AkersTest, RandomFunc5) {
  // Random gate RAND(x, y, z, u, v).
  // Without random care.
  KittyTT func(5);
  KittyTT care(5);
  kitty::create_random(func);
  std::string bitsCare;
  bitsCare.assign(32, '1');
  kitty::create_from_binary_string(care, bitsCare);

  Gate::SignalList inputs;
  Gate::Id outputId;
  BGNet bGNet;

  AkersAlgorithm alg(func, care);
  bGNet.net = alg.run(inputs, outputId);
  bGNet.inputBindings = {inputs[0].node(), inputs[1].node(), inputs[2].node(),
                         inputs[3].node(), inputs[4].node()};
  bGNet.outputBindings = {outputId};

  TruthTable gNetTT = TruthTable::build(bGNet);

  bool equal = areEqualTT(func, care, gNetTT);

  EXPECT_TRUE(equal);
}

TEST(AkersTest, RandomCareFunc6) {
  // Random gate RAND(x, y, z, t, u, v).
  // With random care.
  KittyTT func(6);
  KittyTT care(6);
  kitty::create_random(func);
  kitty::create_random(care);

  Gate::SignalList inputs;
  Gate::Id outputId;
  BGNet bGNet;

  AkersAlgorithm alg(func, care);
  bGNet.net = alg.run(inputs, outputId);
  bGNet.inputBindings = {inputs[0].node(), inputs[1].node(), inputs[2].node(),
                         inputs[3].node(), inputs[4].node(), inputs[5].node()};
  bGNet.outputBindings = {outputId};

  TruthTable gNetTT = TruthTable::build(bGNet);

  bool equal = areEqualTT(func, care, gNetTT);

  EXPECT_TRUE(equal);
}

TEST(AkersTest, RandomFunc6) {
  // Random gate RAND(x, y, z, t, u, v).
  // Without random care.
  KittyTT func(6);
  KittyTT care(6);
  kitty::create_random(func);
  std::string bitsCare;
  bitsCare.assign(64, '1');
  kitty::create_from_binary_string(care, bitsCare);

  Gate::SignalList inputs;
  Gate::Id outputId;
  BGNet bGNet;

  AkersAlgorithm alg(func, care);
  bGNet.net = alg.run(inputs, outputId);
  bGNet.inputBindings = {inputs[0].node(), inputs[1].node(), inputs[2].node(),
                         inputs[3].node(), inputs[4].node(), inputs[5].node()};
  bGNet.outputBindings = {outputId};

  TruthTable gNetTT = TruthTable::build(bGNet);

  bool equal = areEqualTT(func, care, gNetTT);

  EXPECT_TRUE(equal);
}

//===----------------------------------------------------------------------===//
// Competition Tests
//===----------------------------------------------------------------------===//

TEST(AkersTest, CompetitionWithMockturtle) {
  KittyTT func(6);
  KittyTT care(6);
  kitty::create_random(func);
  std::string bitsCare;
  bitsCare.assign(64, '1');
  kitty::create_from_binary_string(care, bitsCare);

  // mockturtle Akers algorithm;
  clock_t start = clock();
  MIGnetwork mockAkers = mockturtle::akers_synthesis<MIGnetwork>(func, care);
  clock_t end = clock();
  double mockturtleTime = (double)(end - start)/CLOCKS_PER_SEC * 1000;

  Gate::SignalList inputs;
  Gate::Id outputId;
  start = clock();
  AkersAlgorithm alg(func, care);
  auto net = alg.run(inputs, outputId);
  end = clock();
  double currentImplTime = (double)(end - start)/CLOCKS_PER_SEC * 1000;

  std::cout << "Time of mockturtle algorithm: " << mockturtleTime << " ms\n";
  std::cout << "Time of current implementation: " << currentImplTime << " ms\n";

  EXPECT_TRUE(currentImplTime <= mockturtleTime);
}
