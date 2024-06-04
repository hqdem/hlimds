//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/design.h"
#include "gate/model/examples.h"
#include "gate/model/generator/matrix_generator.h"
#include "gate/translator/model2.h"
#include "gate/translator/yosys_converter_model2.h"
#include "util/env.h"

#include "gtest/gtest.h"

#include <cstddef>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace eda::gate::model {

static constexpr const char *inPathVerilog = "test/data/gate/parser/verilog";
static constexpr const char *pathFir = "test/data/gate/verilog_to_fir";

using NetID = model::NetID;

static void printNetInfo(const NetID netID) {
  const auto &net = Net::get(netID);
  const size_t inN = net.getInNum();
  const size_t outN = net.getOutNum();
  const size_t innerN = net.getCellNum() - inN - outN;
  const size_t flipFlopN = net.getFlipNum();
  std::cout << "PIs number: " << inN << '\n';
  std::cout << "POs number: " << outN << '\n';
  std::cout << "Inner cells number: " << innerN << '\n';
  std::cout << "FlipFlop number: " << flipFlopN << '\n';
}

static void test(const NetID netID) {
  printNetInfo(netID);

  DesignBuilder builder(netID);

  const size_t subnetNum = builder.getSubnetNum();
  std::cout << "Subnets before uniting: " << subnetNum << '\n';

  std::vector<std::unordered_set<uint64_t>> subnetsFlipFlopSet;
  for (std::size_t i = 0; i < subnetNum; ++i) {
    const auto subnetID = builder.getSubnetID(i);
    const auto &subnet = Subnet::get(subnetID);
    const auto &subnetEntries = subnet.getEntries();
    std::vector<uint64_t> curFlipFlopSet;
    for (const auto &entry : subnetEntries) {
      const auto &cell = entry.cell;
      if (cell.isFlipFlop()) {
        assert(cell.isOut() || cell.isIn());
        curFlipFlopSet.push_back(cell.flipFlopID);
      }
    }
    bool intersect = false;
    for (const auto &curFlipFlop : curFlipFlopSet) {
      for (auto &oldFlipFlopSet : subnetsFlipFlopSet) {
        for (const auto &oldFlipFlop : oldFlipFlopSet) {
          if (curFlipFlop == oldFlipFlop) {
            intersect = true;
            oldFlipFlopSet.insert(curFlipFlopSet.begin(), curFlipFlopSet.end());
            break;
          }
        }
        if (intersect) {
          break;
        }
      }
      if (intersect) {
        break;
      }
    }
    if (!intersect) {
      subnetsFlipFlopSet.push_back({curFlipFlopSet.begin(),
          curFlipFlopSet.end()});
    }
  }
  std::cout << "Subnets after uniting: " << subnetsFlipFlopSet.size() << "\n\n";
}

// TEST(DesignAnalysisTest, AndOr) {
//   const std::string inFileName("andor_test.v");
//   const fs::path inputFullName = eda::env::getHomePath() / pathFir / inFileName;
//   YosysToModel2Config cfg;
//   cfg.debugMode = false;
//   cfg.topModule = "";
//   cfg.files = { inputFullName.c_str() };
//   YosysConverterModel2 translator(cfg);

//   test(translator.getNetID());
// }


TEST(DesignAnalysisTest, Mux2) {
  const std::string inFileName("mux_test.v");
  const fs::path inputFullName = eda::env::getHomePath() / pathFir / inFileName;
  YosysToModel2Config cfg;
  cfg.debugMode = false;
  cfg.topModule = "";
  cfg.files = { inputFullName.c_str() };
  YosysConverterModel2 translator(cfg);

  test(translator.getNetID());
}

TEST(DesignAnalysisTest, RandomNet) {
  const size_t nIn = 5;
  const size_t nOut = 1;
  const size_t nCell = 11;
  const size_t minArity = 1;
  const size_t maxArity = 5;
  const size_t seed = 42;

  MatrixGenerator generator(nCell, nIn, nOut, {DFF_p, NOR}, seed);
  generator.setFaninLim(minArity, maxArity);
  const auto netID = generator.generate();

  test(netID);
}

TEST(DesignAnalysisTest, RandomTriggerNet10) {
  for (size_t i = 0; i < 2; ++i) {
    const size_t minArity = 1;
    const size_t maxArity = 5;
    const unsigned seed = i;

    const size_t nIn = 7;
    const size_t nOut = 7;
    const size_t nCell = 10;

    const auto netID = model::makeTriggerNetRandomMatrix(nIn, nOut, nCell,
        minArity, maxArity, seed);
    test(netID);
  }
}

TEST(DesignAnalysisTest, RandomTriggerNet100) {
  std::srand(0);
  for (size_t i = 0; i < 2; ++i) {
    const size_t minArity = 1;
    const size_t maxArity = 5;
    const unsigned seed = i;

    const size_t nIn = std::rand() % 6 + 10;;
    const size_t nOut = std::rand() % 16 + 30;
    const size_t nCell = std::rand() % 51 + 100;

    const auto netID = model::makeTriggerNetRandomMatrix(nIn, nOut, nCell,
        minArity, maxArity, seed);
    test(netID);
  }
}

TEST(DesignAnalysisTest, RandomTriggerNet1000) {
  std::srand(1);
  for (size_t i = 0; i < 2; ++i) {
    const size_t minArity = 1;
    const size_t maxArity = 5;
    const unsigned seed = i;

    const size_t nIn = std::rand() % 51 + 100;
    const size_t nOut = std::rand() % 151 + 300;
    const size_t nCell = std::rand() % 501 + 1000;

    const auto netID = model::makeTriggerNetRandomMatrix(nIn, nOut, nCell,
        minArity, maxArity, seed);
    test(netID);
  }
}

TEST(DesignAnalysisTest, RandomTriggerNet10000) {
  std::srand(2);
  for (size_t i = 0; i < 2; ++i) {
    const size_t minArity = 1;
    const size_t maxArity = 5;
    const unsigned seed = i;

    const size_t nIn = std::rand() % 501 + 1000;
    const size_t nOut = std::rand() % 1501 + 3000;
    const size_t nCell = std::rand() % 5001 + 10000;

    const auto netID = model::makeTriggerNetRandomMatrix(nIn, nOut, nCell,
        minArity, maxArity, seed);
    test(netID);
  }
}

TEST(DesignAnalysisTest, RandomTriggerNet100000) {
  std::srand(3);
  for (size_t i = 0; i < 2; ++i) {
    const size_t minArity = 1;
    const size_t maxArity = 5;
    const unsigned seed = i;

    const size_t nIn = std::rand() % 501 + 1000;
    const size_t nOut = std::rand() % 1501 + 3000;
    const size_t nLayers = std::rand() % 51 + 100;
    const size_t layerNCellsMin = std::rand() % 6 + 10;
    const size_t layerNCellsMax = std::rand() % 126 + 250;

    const auto netID = model::makeTriggerNetRandomLayer(nIn, nOut, nLayers,
        layerNCellsMin, layerNCellsMax, minArity, maxArity, seed);
    test(netID);
  }
}

} // namespace eda::gate::model
