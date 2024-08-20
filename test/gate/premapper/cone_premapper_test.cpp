//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/sat_checker.h"
#include "gate/premapper/premapper.h"
#include "gate/translator/graphml_test_utils.h"

#include "gtest/gtest.h"

#include <iostream>

namespace eda::gate::premapper {

using Builder = eda::gate::model::SubnetBuilder;

static void runConePremappers(const std::string &file) {
  using Subnet = gate::model::Subnet;

  const auto builder = gate::translator::translateGmlOpenabc(file);
  const auto id = builder->make();
  const auto &old = Subnet::get(id);

  const auto migmapper = getConeMigMapper();
  const auto mig = (migmapper->map(std::make_shared<Builder>(old)))->make(true);
  const auto &migSubnet = Subnet::get(mig);

  const auto xagmapper = getConeXagMapper();
  const auto xag = (xagmapper->map(std::make_shared<Builder>(mig)))->make(true);
  const auto &xagSubnet = Subnet::get(xag);

  const auto xmgmapper = getConeXmgMapper();
  const auto xmg = (xmgmapper->map(std::make_shared<Builder>(xag)))->make(true);
  const auto &xmgSubnet = Subnet::get(xmg);

  const auto aigmapper = getConeAigMapper();
  const auto aig = (aigmapper->map(std::make_shared<Builder>(xag)))->make(true);
  const auto &aigSubnet = Subnet::get(aig);

  // Print characteristics.
  std::cout << "Premapping of " << file << ":" << std::endl;

  std::cout << "Before size: " << old.getCellNum() << std::endl;
  std::cout << "Before depth: " << old.getPathLength().second << std::endl;

  std::cout << "MIG size: " << migSubnet.getCellNum() << std::endl;
  std::cout << "MIG depth: " << migSubnet.getPathLength().second << std::endl;

  std::cout << "XAG size: " << xagSubnet.getCellNum() << std::endl;
  std::cout << "XAG depth: " << xagSubnet.getPathLength().second << std::endl;

  std::cout << "XMG size: " << xmgSubnet.getCellNum() << std::endl;
  std::cout << "XMG depth: " << xmgSubnet.getPathLength().second << std::endl;

  std::cout << "AIG size: " << aigSubnet.getCellNum() << std::endl;
  std::cout << "AIG depth: " << aigSubnet.getPathLength().second << std::endl;

  // Check equivalence.
  gate::debugger::SatChecker &checker = gate::debugger::SatChecker::get();
  EXPECT_TRUE(checker.areEquivalent(old, aigSubnet).equal());

  // Check premapping.
  for (const auto &entry : migSubnet.getEntries()) {
    EXPECT_FALSE(entry.cell.isAnd() || entry.cell.isOr() || entry.cell.isXor());
  }
  for (const auto &entry : xagSubnet.getEntries()) {
    EXPECT_FALSE(entry.cell.isOr() || entry.cell.isMaj());
  }
  for (const auto &entry : xmgSubnet.getEntries()) {
    EXPECT_FALSE(entry.cell.isOr() || entry.cell.isAnd());
  }
  for (const auto &entry : aigSubnet.getEntries()) {
    EXPECT_FALSE(entry.cell.isOr() || entry.cell.isXor() || entry.cell.isMaj());
  }
}

TEST(ConePremapperTest, c1355) {
  runConePremappers("c1355_orig");
}

TEST(ConePremapperTest, sasc) {
  runConePremappers("sasc_orig");
}

} // namespace eda::gate::premapper
