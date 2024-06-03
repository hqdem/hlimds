//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/design.h"
#include "gate/model/examples.h"
#include "gate/model/generator/layer_generator.h"
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

static void test(const NetID netID) {
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

  std::cout << "Subnets after uniting: " << subnetsFlipFlopSet.size() << '\n';
}

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

TEST(DesignAnalysisTest, PicoRV32) {
  const std::string inFileName("picorv.v");
  const fs::path inputFullName = eda::env::getHomePath() / pathFir / inFileName;
  YosysToModel2Config cfg;
  cfg.debugMode = false;
  cfg.topModule = "picorv32";
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

  const auto netID = model::makeFFNetRandomMatrix(nIn, nOut, nCell, minArity,
      maxArity, seed);

  std::cout << Net::get(netID) << '\n';

  test(netID);
}

} // namespace eda::gate::model
