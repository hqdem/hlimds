//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/sat_checker.h"
#include "gate/model/subnet.h"
#include "gate/optimizer/area_optimizer.h"
#include "gate/parser/graphml_parser.h"
#include "util/env.h"

#include "gtest/gtest.h"

#include <filesystem>
#include <string>

using AreaOptimizer = eda::gate::optimizer::AreaOptimizer;
using GraphMlParser = eda::gate::parser::graphml::GraphMlParser;
using LinkList      = eda::gate::model::Subnet::LinkList;
using SatChecker    = eda::gate::debugger::SatChecker;
using Subnet        = GraphMlParser::Subnet;
using SubnetBuilder = eda::gate::model::SubnetBuilder;
using SubnetID      = eda::gate::model::SubnetID;

static constexpr size_t maxArity = 2;
static constexpr size_t cutSize  = 5;

size_t countBufs(const Subnet &subnet) {
  size_t counter = 0;
  const auto &entries = subnet.getEntries();
  for (size_t i = 0; i < entries.size(); ++i) {
    const auto &cell = entries[i].cell;

    if (cell.isBuf()) {
      counter++;
    }

    i += cell.more;
  }

  return counter;
}

void checkAreaOptimizationEquivalence(SubnetID lhs, SubnetID rhs) {
  SatChecker &checker = SatChecker::get();
  const auto &subnet = Subnet::get(lhs);
  const auto &opt = Subnet::get(rhs);

  std::unordered_map<size_t, size_t> map;
  for (size_t i = 0; i < subnet.getInNum(); ++i) {
    map[i] = i;
  }

  for (size_t j = subnet.getOutNum(); j > 0; --j) {
    map[subnet.size() - j] = opt.size() - j;
  }

  EXPECT_TRUE(checker.areEquivalent(lhs, rhs, map).equal());
}

void runAreaOptimization(SubnetID subnetId) {
  const auto &subnet = Subnet::get(subnetId);
  // Builder for optimization.
  SubnetBuilder builder;
  const auto inputs = builder.addInputs(subnet.getInNum());
  const auto outputs = builder.addSubnet(subnetId, inputs);
  builder.addOutputs(outputs);
  // Area optimization.
  AreaOptimizer areaOptimizer(builder, maxArity, cutSize);
  areaOptimizer.optimize();
  auto optimizedId = builder.make();
  const Subnet &optimized = Subnet::get(optimizedId);

  size_t nBufs = countBufs(optimized);

  EXPECT_TRUE(optimized.size() - nBufs <= subnet.size());

  // Equivalence checking.
  checkAreaOptimizationEquivalence(subnetId, optimizedId);
}

void runAreaOptimization(std::string str) {
  using path = std::filesystem::path;
  // File opening.
  std::string fileName = str += ".bench.graphml";
  const path dir = path("test") / "data" / "gate" / "parser"
      / "graphml" / "OpenABC" / "graphml_openabcd";
  const path home = eda::env::getHomePath();
  const path file = home / dir / fileName;
  // Parsing.
  GraphMlParser parser;
  const SubnetID subnetId = parser.parse(file.string()).make();
  // Optimize.
  runAreaOptimization(subnetId);
}

TEST(AreaOptimizerTest, sasc) {
  runAreaOptimization("sasc_orig");
}

TEST(AreaOptimizerTest, ssPcm) {
  runAreaOptimization("ss_pcm_orig");
}

TEST(AreaOptimizerTest, usbPhy) {
  runAreaOptimization("usb_phy_orig");
}
