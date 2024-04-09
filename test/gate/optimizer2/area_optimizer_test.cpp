//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger2/sat_checker2.h"
#include "gate/model2/subnet.h"
#include "gate/optimizer2/area_optimizer.h"
#include "gate/parser/graphml_to_subnet.h"

#include "gtest/gtest.h"

#include <filesystem>
#include <string>

using AreaOptimizer       = eda::gate::optimizer2::AreaOptimizer;
using GraphMlSubnetParser = eda::gate::parser::graphml::GraphMlSubnetParser;
using LinkList            = eda::gate::model::Subnet::LinkList;
using SatChecker2         = eda::gate::debugger2::SatChecker2;
using Subnet              = GraphMlSubnetParser::Subnet;
using SubnetBuilder       = eda::gate::model::SubnetBuilder;
using SubnetID            = eda::gate::model::SubnetID;

static constexpr size_t maxArity = 2;
static constexpr size_t cutSize  = 5;

void checkAreaOptimizationEquivalence(SubnetID lhs, SubnetID rhs) {
  SatChecker2 &checker = SatChecker2::get();
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

  EXPECT_TRUE(optimized.size() <= subnet.size());

  // Equivalence checking.
  checkAreaOptimizationEquivalence(subnetId, optimizedId);
}

void runAreaOptimization(std::string str) {
  using path = std::filesystem::path;
  // File opening.
  std::string fileName = str += ".bench.graphml";
  const path dir = path("test") / "data" / "gate" / "parser"
      / "graphml" / "OpenABC" / "graphml_openabcd";
  const path home = std::string(getenv("UTOPIA_HOME"));
  const path file = home / dir / fileName;
  // Parsing.
  GraphMlSubnetParser parser;
  const SubnetID subnetId = parser.parse(file.string());
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
