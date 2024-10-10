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

void cloneTest(const SubnetID id) {
  using options::BDD;
  using options::FRAIG;
  using options::RND;
  using options::SAT;

  BaseChecker::CellToCell map;
  const auto &subnet = model::Subnet::get(id);
  for (size_t i = 0; i < subnet.getEntries().size(); ++i) {
    map[i] = i;
  }

  // RND checker (non-exhaustive mode)
  static_cast<RndChecker&>(BaseChecker::getChecker(RND)).setExhaustive(false);
  static_cast<RndChecker&>(BaseChecker::getChecker(RND)).setTries(100);
  EXPECT_TRUE(BaseChecker::getChecker(RND).areEquivalent(id, id, map).isUnknown());

  // RND checker (exhaustive mode)
  static_cast<RndChecker&>(BaseChecker::getChecker(RND)).setExhaustive(true);
  EXPECT_TRUE(BaseChecker::getChecker(RND).areEquivalent(id, id, map).equal());

  // BDD checker
  EXPECT_TRUE(BaseChecker::getChecker(BDD).areEquivalent(id, id, map).equal());

  // FRAIG checker
  EXPECT_TRUE(BaseChecker::getChecker(FRAIG).areEquivalent(id, id, map).equal());

  // SAT checker
  EXPECT_TRUE(BaseChecker::getChecker(SAT).areEquivalent(id, id, map).equal());
}

TEST(LecCloneTest, MatrixGenerator) {
  const size_t nIn      = 10;
  const size_t nOut     = 10;
  const size_t nCell    = 10;
  const size_t minArity = 5;
  const size_t maxArity = 5;
  const size_t nSubnet  = 5;

  for (size_t i = 0; i < nSubnet; ++i) {
    cloneTest(makeSubnetRandomMatrix(nIn, nOut, nCell, minArity, maxArity, i));
  }
}

TEST(LecCloneTest, 3AndOrXor) {
  cloneTest(makeSubnet3AndOrXor());
}

TEST(LecCloneTest, XorNorAndAndOr) {
  cloneTest(makeSubnetXorNorAndAndOr());
}

TEST(LecCloneTest, XorOrXor) {
  cloneTest(makeSubnetXorOrXor());
}

TEST(LecCloneTest, AndOrXor) {
  cloneTest(makeSubnetAndOrXor());
}

TEST(LecCloneTest, 4AndOr) {
  cloneTest(makeSubnet4AndOr());
}

TEST(LecCloneTest, OnlyInputsAndOutput) {
  model::SubnetBuilder builder;
  const auto input = builder.addInput();
  builder.addOutput(input);
  auto subnetID = builder.make();

  model::SubnetBuilder builder2;
  const auto input2 = builder2.addInput();
  const auto const1 = builder2.addCell(model::ONE);
  const auto and2 = builder2.addCell(model::AND, const1, input2);
  builder2.addOutput(and2);
  auto subnetID2 = builder2.make();

  EXPECT_TRUE(BaseChecker::getChecker(options::SAT).
              areEquivalent(subnetID, subnetID2).equal());
}

} // namespace eda::gate::debugger
