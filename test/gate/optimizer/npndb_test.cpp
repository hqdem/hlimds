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

static bool areEquivalent(const BoundGNet &lhs,
                          const BoundGNet &rhs) {
  return NPNDatabase::buildTT(lhs) == NPNDatabase::buildTT(rhs);
}

bool transformTest(BoundGNet bnet) {
  using TT = NPNDatabase::TT;

  TT tt = NPNDatabase::buildTT(bnet);
  const auto config = kitty::exact_npn_canonization(tt);
  TT canonTT = eda::utils::getTT(config);
  NPNDatabase::NPNTransformation t = eda::utils::getTransformation(config);
  NPNDatabase::npnTransformInplace(bnet, t);
  tt = NPNDatabase::buildTT(bnet);
  return tt == canonTT;
}

bool npnDatabaseTest(BoundGNet bnet) {
  NPNDatabase npndb;
  npndb.push(bnet);
  uint16_t negationMask = 5;
  NPNDatabase::InputPermutation permutation;
  for (size_t i = 0; i < bnet.inputBindings.size(); i++) {
    permutation.push_back(bnet.inputBindings.size() - i - 1);
  }
  NPNDatabase::NPNTransformation t = {negationMask, permutation};
  BoundGNet bnet1 = NPNDatabase::npnTransform(bnet, t);
  if (std::get<0>(npndb.get(bnet1)).empty()) {
    return false;
  }
  auto result = npndb.get(bnet1);
  BoundGNet bnet2 = NPNDatabase::npnTransform(std::get<0>(result)[0],
                                              std::get<1>(result));

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
