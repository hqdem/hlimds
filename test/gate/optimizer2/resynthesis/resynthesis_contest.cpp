//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/analyzer/simulation_estimator.h"
#include "gate/optimizer2/resynthesis/akers.h"
#include "gate/optimizer2/resynthesis/bidecomposition.h"
#include "gate/optimizer2/resynthesis/cascade.h"
#include "gate/optimizer2/resynthesis/de_micheli.h"
#include "gate/optimizer2/resynthesis/isop.h"
#include "gate/optimizer2/resynthesis/reed_muller.h"
#include "gate/optimizer2/synthesizer.h"

#include "gtest/gtest.h"
#include "kitty/constructors.hpp"
#include "kitty/print.hpp"

#include <ctime>
#include <filesystem>
#include <fstream>
#include <ios>
#include <vector>

using AkersAlgorithm    = eda::gate::optimizer2::resynthesis::AkersAlgorithm;
using BiDecomposition   = eda::gate::optimizer2::resynthesis::BiDecomposition;
using CascadeMethod     = eda::gate::optimizer2::resynthesis::Cascade;
using DeMicheli         = eda::gate::optimizer2::resynthesis::DeMicheli;
using DynTruthTable     = kitty::dynamic_truth_table;
using MinatoMorrealeAlg = eda::gate::optimizer2::resynthesis::MinatoMorrealeAlg;
using ReedMullerAlg     = eda::gate::optimizer2::resynthesis::ReedMuller;
using SimEstimator      = eda::gate::analyzer::SimulationEstimator;
using Subnet            = eda::gate::model::Subnet;
using SubnetID          = eda::gate::model::SubnetID;
using SynthTable        = eda::gate::optimizer2::Synthesizer<DynTruthTable>;

constexpr unsigned RAND3_TT_NUM  = 8;
constexpr unsigned RAND4_TT_NUM  = 16;
constexpr unsigned RAND5_TT_NUM  = 32;
constexpr unsigned RAND6_TT_NUM  = 64;
constexpr unsigned RAND7_TT_NUM  = 128;
constexpr unsigned RAND8_TT_NUM  = 256;
constexpr unsigned RAND9_TT_NUM  = 512;
constexpr unsigned RAND10_TT_NUM = 1024;

/// Defines the names of the resynthesis algorithms.
enum class Algorithm {
  /// Akers algorithm.
  Akers,
  /// The bi-decomposition method.
  BiDecomposition,
  /// The cascade method.
  Cascade,
  /// De Micheli algorithm.
  DeMicheli,
  /// Minato-Morreale algorithm.
  MinatoMorreale,
  /// Reed-Muller algorithm.
  ReedMuller
};

void writeLogs(std::ofstream &file, const DynTruthTable &table, Algorithm alg,
               const SubnetID &id, clock_t st, clock_t end, bool err = false) {

  // Algorithm.
  switch (alg) {
    case Algorithm::Akers:
      file << "Akers,";
    break;
    case Algorithm::BiDecomposition:
      file << "Bi-decomposition,";
    break;
    case Algorithm::Cascade:
      file << "Cascade,";
    break;
    case Algorithm::DeMicheli:
      file << "De Micheli";
    break;
    case Algorithm::MinatoMorreale:
      file << "Minato-Morreale,";
    break;
    case Algorithm::ReedMuller:
      file << "Reed-Muller,";
    break;
  }
  // Truth table.
  file << kitty::to_hex(table) << ',';
  // Inputs.
  file << table.num_vars() << ',';
  // Error.
  if (err) {
    file << "ERROR,ERROR,ERROR,ERROR" << std::endl;
    return;
  }
  // Inner gates.
  const Subnet subnet = Subnet::get(id);
  file << subnet.size() - subnet.getInNum() - subnet.getOutNum() << ',';
  // Depth.
  const auto length = subnet.getPathLength();
  file << length.second << ',';
  // Time (ms).
  file << (double)(end - st)/CLOCKS_PER_SEC * 1000 << ',';
  // Toggle rate
  SimEstimator simEstimator;
  file << simEstimator.estimate(subnet).getActivitySum() << std::endl;
}

void runTest(const DynTruthTable &table) {
  // Open the file for logs.
  std::string fileName = "resynthesis_contest.csv";
  namespace fs = std::filesystem;
  const fs::path homePath = std::string(getenv("UTOPIA_HOME"));
  const fs::path outputPath = "output/test/resynthesis";
  fs::path file = homePath / outputPath / fileName;
  fs::path dir = file.parent_path();
  if (!fs::exists(dir)) {
    fs::create_directories(dir);
  }
  std::ofstream fout(file, std::ios_base::app);
  std::ifstream fin(file);
  if (fout.is_open() && "Cannot open the file for tests logs!");
  if (fin.peek() == EOF) {
    fout << "Algorithm,Truth table,Inputs,Inner gates,Depth,Time (ms),"
        << "Switching activity" << std::endl;
  } else {
    fout.seekp(0, std::ios_base::end);
  }
  fin.close();

  // Algorithms registry.
  std::vector<SynthTable*> registry;
  registry.push_back(new AkersAlgorithm());
  registry.push_back(new BiDecomposition());
  registry.push_back(new CascadeMethod());
  registry.push_back(new DeMicheli());
  registry.push_back(new MinatoMorrealeAlg());
  registry.push_back(new ReedMullerAlg());
  // Launching.
  for (size_t i = 0; i < registry.size(); i++) {
    /// TODO: decide, what may be used instead of "assert" to indicate an error.
    if ((i == 0) && (table.num_vars() > 7)) {
      writeLogs(fout, table, Algorithm::Akers, 0, 0, 0, true);
      continue;
    }
    if ((i == 1) && (table.num_vars() > 8)) {
      writeLogs(fout, table, Algorithm::BiDecomposition, 0, 0, 0, true);
      continue;
    }
    clock_t start = clock();
    const auto id = registry[i]->synthesize(table);
    clock_t end = clock();
    if ((i == 3) && (id == eda::gate::model::OBJ_NULL_ID)) {
      writeLogs(fout, table, Algorithm::DeMicheli, 0, 0, 0, true);
      continue;
    }
    writeLogs(fout, table, static_cast<Algorithm>(i), id, start, end);
  }
  // Delete objects of the registry.
  for (size_t i = 0; i < registry.size(); i++) {
    delete registry[i];
  }

  fout.close();
}

//===----------------------------------------------------------------------===//
// Functions of 2 arguments.
//===----------------------------------------------------------------------===//

TEST(ResynthesisContest, Spec2) {
  DynTruthTable table(2);
  // Const ZERO.
  kitty::create_from_binary_string(table, "0000");
  runTest(table);
  // Const ONE.
  kitty::create_from_binary_string(table, "1111");
  runTest(table);
  // OR(x, y).
  kitty::create_from_binary_string(table, "1110");
  runTest(table);
  // AND(x, y).
  kitty::create_from_binary_string(table, "1000");
  runTest(table);
  // XOR(x, y).
  kitty::create_from_binary_string(table, "0110");
  runTest(table);
  // NOR(x, y).
  kitty::create_from_binary_string(table, "0001");
  runTest(table);
  // NAND(x, y).
  kitty::create_from_binary_string(table, "0111");
  runTest(table);
  // XNOR(x, y).
  kitty::create_from_binary_string(table, "1001");
  runTest(table);
}

//===----------------------------------------------------------------------===//
// Functions of 3 arguments.
//===----------------------------------------------------------------------===//

TEST(ResynthesisContest, RAND3) {
  DynTruthTable table(3);
  for (size_t i = 0; i < RAND3_TT_NUM; i++) {
    kitty::create_random(table);
    runTest(table);
  }
}

//===----------------------------------------------------------------------===//
// Functions of 4 arguments.
//===----------------------------------------------------------------------===//

TEST(ResynthesisContest, RAND4) {
  DynTruthTable table(4);
  for (size_t i = 0; i < RAND4_TT_NUM; i++) {
    kitty::create_random(table);
    runTest(table);
  }
}

//===----------------------------------------------------------------------===//
// Functions of 5 arguments.
//===----------------------------------------------------------------------===//

TEST(ResynthesisContest, RAND5) {
  DynTruthTable table(5);
  for (size_t i = 0; i < RAND5_TT_NUM; i++) {
    kitty::create_random(table);
    runTest(table);
  }
}

//===----------------------------------------------------------------------===//
// Functions of 6 arguments.
//===----------------------------------------------------------------------===//

TEST(ResynthesisContest, RAND6) {
  DynTruthTable table(6);
  for (size_t i = 0; i < RAND6_TT_NUM; i++) {
    kitty::create_random(table);
    runTest(table);
  }
}

//===----------------------------------------------------------------------===//
// Functions of 7 arguments.
//===----------------------------------------------------------------------===//

TEST(ResynthesisContest, Spec7) {
  DynTruthTable table(7);
  // Infinity while() in mockturtle::akers_synthesis(table).
  kitty::create_from_hex_string(table, "86499DA989F5B9969EC93C064D224C61");
  runTest(table);
}

TEST(ResynthesisContest, RAND7) {
  DynTruthTable table(7);
  for (size_t i = 0; i < RAND7_TT_NUM; i++) {
    kitty::create_random(table);
    runTest(table);
  }
}

//===----------------------------------------------------------------------===//
// Functions of 8 arguments.
//===----------------------------------------------------------------------===//

TEST(ResynthesisContest, RAND8) {
  DynTruthTable table(8);
  for (size_t i = 0; i < RAND8_TT_NUM; i++) {
    kitty::create_random(table);
    runTest(table);
  }
}

//===----------------------------------------------------------------------===//
// Functions of 9 arguments.
//===----------------------------------------------------------------------===//

TEST(ResynthesisContest, RAND9) {
  DynTruthTable table(9);
  for (size_t i = 0; i < RAND9_TT_NUM; i++) {
    kitty::create_random(table);
    runTest(table);
  }
}

//===----------------------------------------------------------------------===//
// Functions of 10 arguments.
//===----------------------------------------------------------------------===//

TEST(ResynthesisContest, RAND10) {
  DynTruthTable table(10);
  for (size_t i = 0; i < RAND10_TT_NUM; i++) {
    kitty::create_random(table);
    runTest(table);
  }
}
