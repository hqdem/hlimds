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
#include "util/assert.h"
#include "util/env.h"

#include "gtest/gtest.h"

#include <filesystem>
#include <string>

using AreaOptimizer = eda::gate::optimizer::AreaOptimizer;
using GraphMlParser = eda::gate::parser::graphml::GraphMlParser;
using SatChecker    = eda::gate::debugger::SatChecker;
using Subnet        = GraphMlParser::Subnet;
using SubnetBuilder = eda::gate::model::SubnetBuilder;
using SubnetID      = eda::gate::model::SubnetID;

static constexpr size_t maxArity = 2;
static constexpr size_t cutSize  = 5;

void checkAreaOptimizerEquivalence(SubnetID lhs, SubnetID rhs) {
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

void runAreaOptimizer(SubnetID subnetId) {
  const auto &subnet = Subnet::get(subnetId);
  // Builder for optimization.
  SubnetBuilder builder(subnetId);
  // Area optimization.
  AreaOptimizer areaOptimizer(builder, maxArity, cutSize);
  areaOptimizer.optimize();
  // Make subnet w/o BUFs.
  auto optimizedId = builder.make(true);
  const Subnet &optimized = Subnet::get(optimizedId);

  EXPECT_TRUE(optimized.size() <= subnet.size());

  // Equivalence checking.
  checkAreaOptimizerEquivalence(subnetId, optimizedId);
}

void runAreaOptimizer(std::string filename) {
  using path = std::filesystem::path;
  // File opening.
  filename += ".bench.graphml";
  const path dir = path("test") / "data" / "gate" / "parser"
      / "graphml" / "OpenABC" / "graphml_openabcd";
  const path home = eda::env::getHomePath();
  const path file = home / dir / filename;

  uassert(std::filesystem::exists(file.string()),
                                 "File " << file <<
                                 " doesn't exist" << std::endl);

  // Parsing.
  GraphMlParser parser;
  const SubnetID subnetId = parser.parse(file.string()).make();
  // Optimize.
  runAreaOptimizer(subnetId);
}

TEST(AreaOptimizerTest, sasc) {
  runAreaOptimizer("sasc_orig");
}

TEST(AreaOptimizerTest, ssPcm) {
  runAreaOptimizer("ss_pcm_orig");
}

TEST(AreaOptimizerTest, usbPhy) {
  runAreaOptimizer("usb_phy_orig");
}
