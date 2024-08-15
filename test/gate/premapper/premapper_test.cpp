//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/sat_checker.h"
#include "gate/optimizer/pass.h"
#include "gate/translator/graphml_test_utils.h"

#include "gtest/gtest.h"

#include <iostream>

namespace eda::gate::premapper {

static void runMigMapper(const std::string &file) {
  using Subnet = gate::model::Subnet;

  const auto builder = gate::translator::translateGmlOpenabc(file);
  const auto id = builder->make();
  const auto &subnet = Subnet::get(id);
  
  const auto pass = optimizer::mig();
  const auto premappedID = pass->transform(id);
  const auto &premapped = Subnet::get(premappedID);

  // Print characteristics.
  std::cout << "Premapping of " << file << ":" << std::endl;
  std::cout << "Before size: " << subnet.getCellNum() << std::endl;
  std::cout << "Before depth: " << subnet.getPathLength().second << std::endl;
  std::cout << "After size: " << premapped.getCellNum() << std::endl;
  std::cout << "After depth: " << premapped.getPathLength().second << std::endl;

  // Check equivalence.
  gate::debugger::SatChecker &checker = gate::debugger::SatChecker::get();
  EXPECT_TRUE(checker.areEquivalent(id, premappedID).equal());

  // Check premapping.
  for (const auto &entry : premapped.getEntries()) {
    EXPECT_FALSE(entry.cell.isAnd() || entry.cell.isOr() || entry.cell.isXor());
  }
}

TEST(MigMapperTest, c1355) {
  runMigMapper("c1355_orig");
}

TEST(MigMapperTest, sasc) {
  runMigMapper("sasc_orig");
}

} // namespace eda::gate::premapper
