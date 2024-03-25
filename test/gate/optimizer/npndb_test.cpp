//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/examples.h"
#include "gate/model/gnet_test.h"
#include "gate/optimizer/npndb.h"
#include "util/kitty_utils.h"

#include "gtest/gtest.h"

using namespace eda::gate::model;
using namespace eda::gate::optimizer;
using namespace eda::utils;

static bool areEquivalent(const BoundGNet &lhs,
                          const BoundGNet &rhs) {
  return buildTT(lhs) == buildTT(rhs);
}

bool transformTest(BoundGNet bnet) {
  using TT = NPNDatabase::TT;

  TT tt = buildTT(bnet);
  const auto config = kitty::exact_npn_canonization(tt);
  TT canonTT = eda::utils::getTT(config);
  NPNTransformation t = eda::utils::getTransformation(config);
  npnTransformInplace(bnet, t);
  tt = buildTT(bnet);
  return tt == canonTT;
}

bool npnDatabaseTest(BoundGNet bnet) {
  NPNDatabase npndb;
  npndb.push(bnet);
  uint16_t negationMask = 5;
  NPNTransformation::InputPermutation permutation;
  for (size_t i = 0; i < bnet.inputBindings.size(); i++) {
    permutation.push_back(bnet.inputBindings.size() - i - 1);
  }
  NPNTransformation t = {negationMask, permutation};
  BoundGNet bnet1 = npnTransform(bnet, t);
  auto res = npndb.get(bnet1);
  assert(!res.isEnd());
  BoundGNet bnet2 = res.get();

  return areEquivalent(bnet1, bnet2);
}

TEST(NPNDB, TransformTest) {
  EXPECT_TRUE(transformTest(makeTestBgnet1()));
  EXPECT_TRUE(transformTest(makeTestBgnet2()));
  EXPECT_TRUE(transformTest(makeTestBgnet3()));
}

TEST(NPNDB, npnDatabaseTest) {
  EXPECT_TRUE(npnDatabaseTest(makeTestBgnet1()));
  EXPECT_TRUE(npnDatabaseTest(makeTestBgnet2()));
  EXPECT_TRUE(npnDatabaseTest(makeTestBgnet3()));
}

