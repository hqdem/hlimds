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

void runResubstitutor(const std::string &file) {
  const auto subnetID = eda::gate::translator::translateGmlOpenabc(file)->make();
  const auto &subnet = Subnet::get(subnetID);
  // Builder for optimization.
  auto builder = std::make_shared<SubnetBuilder>(subnetID);
  // Area optimization.
  Resubstitutor resub("rs", 8, 3, false, false);
  resub.transform(builder);
  auto optimizedId = builder->make(true);
  const Subnet &optimized = Subnet::get(optimizedId);

  std::cout << "Size before: " << subnet.size() << std::endl;
  std::cout << "Size after: " << optimized.size() << std::endl;

  EXPECT_TRUE(optimized.size() <= subnet.size());

  // Equivalence checking.
  SatChecker &checker = SatChecker::get();
  EXPECT_TRUE(checker.areEquivalent(subnetID, optimizedId).equal());
}

TEST(ResubstitutorTest, C7552) {
  runResubstitutor("c7552_orig");
}

TEST(ResubstitutorTest, C5315) {
  runResubstitutor("c5315_orig");
}
