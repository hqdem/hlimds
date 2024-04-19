//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger2/bdd_checker2.h"
#include "gate/debugger2/rnd_checker2.h"
#include "gate/model2/examples.h"

#include "gtest/gtest.h"

using namespace eda::gate::model;

namespace eda::gate::debugger2 {

void lecTest(const SubnetID id) {
  using options::BDD;
  using options::RND;
  CellToCell map;
  const Subnet &subnet = Subnet::get(id);
  for (size_t i = 0; i < subnet.getEntries().size(); ++i) {
    map[i] = i;
  }
  static_cast<RndChecker2&>(getChecker(RND)).setExhaustive(false);
  static_cast<RndChecker2&>(getChecker(RND)).setTries(100);
  EXPECT_TRUE(getChecker(RND).areEquivalent(id, id, map).isUnknown());

  static_cast<RndChecker2&>(getChecker(RND)).setExhaustive(true);
  EXPECT_TRUE(getChecker(RND).areEquivalent(id, id, map).equal());
  EXPECT_TRUE(getChecker(BDD).areEquivalent(id, id, map).equal());
}

TEST(LecTest, MatrixGenerator) {
  const size_t nIn      = 10;
  const size_t nOut     = 10;
  const size_t nCell    = 200;
  const size_t minArity = 2;
  const size_t maxArity = 7;
  const size_t nSubnet  = 40;

  for (size_t i = 0; i < nSubnet; ++i) {
    lecTest(makeRandomSubnetMatrix(nIn, nOut, nCell, minArity, maxArity, i));
  }
}

TEST(LecTest, 3AndOrXor) {
  lecTest(make3AndOrXor());
}

TEST(LecTest, XorNorAndAndOr) {
  lecTest(makeXorNorAndAndOr());
}

TEST(LecTest, XorOrXor) {
  lecTest(makeXorOrXor());
}

TEST(LecTest, AndOrXor) {
  lecTest(makeAndOrXor());
}

TEST(LecTest, 4AndOr) {
  lecTest(make4AndOr());
}

} // namespace eda::gate::debugger2
