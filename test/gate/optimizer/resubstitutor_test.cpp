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
#include "gate/translator/graphml_test_utils.h"
#include "util/assert.h"
#include "util/env.h"

#include "gtest/gtest.h"

#include <filesystem>
#include <string>

using Resubstitutor = eda::gate::optimizer::Resubstitutor;
using SatChecker    = eda::gate::debugger::SatChecker;
using Subnet        = eda::gate::model::Subnet;
using SubnetBuilder = eda::gate::model::SubnetBuilder;
using SubnetID      = eda::gate::model::SubnetID;

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
  auto builder = std::make_shared<SubnetBuilder>(subnetId);
  // Area optimization.
  Resubstitutor resub("rs");
  resub.transform(builder);
  auto optimizedId = builder->make(true);
  const Subnet &optimized = Subnet::get(optimizedId);

  std::cout << "Before: " << subnet.size() << std::endl;
  std::cout << "After: " << optimized.size() << std::endl;

  EXPECT_TRUE(optimized.size() <= subnet.size());

  // Equivalence checking.
  checkResubstitutionEquivalence(subnetId, optimizedId);
}

void runResubstitutor(std::string file) {
  // Parsing.
  const auto builder = eda::gate::translator::translateGmlOpenabc(file);
  // Optimize.
  runResubstitutor(builder->make());
}

TEST(ResubstitutorTest, C7552) {
  runResubstitutor("c7552_orig");
}

TEST(ResubstitutorTest, C5315) {
  runResubstitutor("c5315_orig");
}
