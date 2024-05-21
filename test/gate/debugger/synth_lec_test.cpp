//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/base_checker.h"
#include "gate/debugger/bdd_checker.h"
#include "gate/debugger/fraig_checker.h"
#include "gate/debugger/rnd_checker.h"
#include "gate/model/examples.h"

#include "gtest/gtest.h"

using namespace eda::gate::model;

namespace eda::gate::debugger {

void lecTest(const SubnetID id) {
  using options::BDD;
  using options::FRAIG;
  using options::RND;

  BaseChecker::CellToCell map;
  const auto &subnet = model::Subnet::get(id);
  for (size_t i = 0; i < subnet.getEntries().size(); ++i) {
    map[i] = i;
  }

  static_cast<RndChecker&>(BaseChecker::getChecker(RND)).setExhaustive(false);
  static_cast<RndChecker&>(BaseChecker::getChecker(RND)).setTries(100);
  EXPECT_TRUE(BaseChecker::getChecker(RND).areEquivalent(id, id, map).isUnknown());

  static_cast<RndChecker&>(BaseChecker::getChecker(RND)).setExhaustive(true);
  EXPECT_TRUE(BaseChecker::getChecker(RND).areEquivalent(id, id, map).equal());
  EXPECT_TRUE(BaseChecker::getChecker(BDD).areEquivalent(id, id, map).equal());
  EXPECT_TRUE(BaseChecker::getChecker(FRAIG).areEquivalent(id, id, map).equal());
}

TEST(LecTest, MatrixGenerator) {
  const size_t nIn      = 10;
  const size_t nOut     = 10;
  const size_t nCell    = 100;
  const size_t minArity = 2;
  const size_t maxArity = 5;
  const size_t nSubnet  = 40;

  for (size_t i = 0; i < nSubnet; ++i) {
    lecTest(makeSubnetRandomMatrix(nIn, nOut, nCell, minArity, maxArity, i));
  }
}

TEST(LecTest, 3AndOrXor) {
  lecTest(makeSubnet3AndOrXor());
}

TEST(LecTest, XorNorAndAndOr) {
  lecTest(makeSubnetXorNorAndAndOr());
}

TEST(LecTest, XorOrXor) {
  lecTest(makeSubnetXorOrXor());
}

TEST(LecTest, AndOrXor) {
  lecTest(makeSubnetAndOrXor());
}

TEST(LecTest, 4AndOr) {
  lecTest(makeSubnet4AndOr());
}

} // namespace eda::gate::debugger
