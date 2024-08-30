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
using BuilderPtr = std::shared_ptr<Builder>;
using CellSymbol = eda::gate::model::CellSymbol;
using Subnet = eda::gate::model::Subnet;

static void runConePremappers(const std::string &file) {
  const auto builder = gate::translator::translateGmlOpenabc(file);
  const auto id = builder->make();
  const auto &old = Subnet::get(id);

  const auto migmapper = getConeMigMapper();
  const auto xagmapper = getConeXagMapper();
  const auto xmgmapper = getConeXmgMapper();
  const auto aigmapper = getConeAigMapper();

  const auto mig = (migmapper->map(std::make_shared<Builder>(old)))->make(true);
  const auto xag = (xagmapper->map(std::make_shared<Builder>(mig)))->make(true);
  const auto xmg = (xmgmapper->map(std::make_shared<Builder>(xag)))->make(true);
  const auto aig = (aigmapper->map(std::make_shared<Builder>(xag)))->make(true);

  const auto &migSubnet = Subnet::get(mig);
  const auto &xagSubnet = Subnet::get(xag);
  const auto &xmgSubnet = Subnet::get(xmg);
  const auto &aigSubnet = Subnet::get(aig);

  // Print characteristics.
  std::cout << "Premapping of " << file << ":" << std::endl;

  std::cout << "Original size:  " << old.getCellNum()           << std::endl;
  std::cout << "Original depth: " << old.getPathLength().second << std::endl;

  std::cout << "MIG size:  " << migSubnet.getCellNum()           << std::endl;
  std::cout << "MIG depth: " << migSubnet.getPathLength().second << std::endl;
  std::cout << "XAG size:  " << xagSubnet.getCellNum()           << std::endl;
  std::cout << "XAG depth: " << xagSubnet.getPathLength().second << std::endl;
  std::cout << "XMG size:  " << xmgSubnet.getCellNum()           << std::endl;
  std::cout << "XMG depth: " << xmgSubnet.getPathLength().second << std::endl;
  std::cout << "AIG size:  " << aigSubnet.getCellNum()           << std::endl;
  std::cout << "AIG depth: " << aigSubnet.getPathLength().second << std::endl;

  // Check equivalence.
  gate::debugger::SatChecker &checker = gate::debugger::SatChecker::get();
  EXPECT_TRUE(checker.areEquivalent(old, aigSubnet).equal());
}

TEST(ConePremapperTest, ArityChecking) {
  const size_t nIn = 5;

  const auto original = std::make_shared<Builder>();
  const auto links = original->addInputs(nIn);

  const auto link = original->addCell(CellSymbol::AND, links);
  original->addOutput(link);

  const auto aigmapper = getConeAigMapper();
  const auto premapped = aigmapper->map(original);

  const auto &subnet = Subnet::get(premapped->make());
  // Check arity.
  for (const auto &entry : subnet.getEntries()) {
    EXPECT_TRUE(entry.cell.arity <= 2);
  }
}

TEST(ConePremapperTest, EquivalenceChecking) {
  runConePremappers("sasc_orig");
}

} // namespace eda::gate::premapper
