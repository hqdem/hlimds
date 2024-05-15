//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/sat_checker.h"
#include "gate/model/subnet.h"
#include "gate/optimizer/resubstitutor.h"
#include "gate/parser/graphml_to_subnet.h"

#include "gtest/gtest.h"

#include <filesystem>
#include <string>

using GraphMlSubnetParser = eda::gate::parser::graphml::GraphMlSubnetParser;
using LinkList            = eda::gate::model::Subnet::LinkList;
using Resubstitutor       = eda::gate::optimizer::Resubstitutor;
using SatChecker          = eda::gate::debugger::SatChecker;
using Subnet              = GraphMlSubnetParser::Subnet;
using SubnetBuilder       = eda::gate::model::SubnetBuilder;
using SubnetID            = eda::gate::model::SubnetID;

void checkResubstitutionEquivalence(SubnetID lhs, SubnetID rhs) {
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

void runResubstitutor(SubnetID subnetId) {
  const auto &subnet = Subnet::get(subnetId);
  // Builder for optimization.
  SubnetBuilder builder(subnetId);
  // Area optimization.
  Resubstitutor resub;
  resub.transform(builder);
  auto optimizedId = builder.make();
  const Subnet &optimized = Subnet::get(optimizedId);

  std::cout << "Before: " << subnet.size() << std::endl;
  std::cout << "After: " << optimized.size() << std::endl;

  EXPECT_TRUE(optimized.size() <= subnet.size());

  // Equivalence checking.
  checkResubstitutionEquivalence(subnetId, optimizedId);
}

void runResubstitutor(std::string filename) {
  using path = std::filesystem::path;
  // File opening.
  filename += ".bench.graphml";
  const path dir = path("test") / "data" / "gate" / "parser"
      / "graphml" / "OpenABC" / "graphml_openabcd";
  const path home = std::string(getenv("UTOPIA_HOME"));
  const path file = home / dir / filename;
  // Parsing.
  GraphMlSubnetParser parser;
  const SubnetID subnetId = parser.parse(file.string());
  // Optimize.
  runResubstitutor(subnetId);
}

TEST(ResubstitutorTest, Bc0) {
  runResubstitutor("bc0_orig");
}

TEST(ResubstitutorTest, C5315) {
  runResubstitutor("c5315_orig");
}
