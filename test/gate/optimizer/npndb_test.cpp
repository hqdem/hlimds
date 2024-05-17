//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/examples.h"
#include "gate/optimizer/npndb.h"

#include "gtest/gtest.h"

using namespace eda::gate::model;
using namespace eda::gate::optimizer;
using namespace eda::utils;

static size_t inputsCount(const Subnet &subnet) {
  size_t result = 0;
  for (const auto &entry : subnet.getEntries()) {
    if (!entry.cell.isIn()) {
      break;
    }
    result++;
  }
  return result;
}

static bool areEquivalent(const Subnet &lhs,
                          const Subnet &rhs) {
  return evaluate(lhs) == evaluate(rhs);
}

static bool transformTest(const SubnetID &id) {
  using TT = NPNDatabase2::TT;

  TT tt = evaluate(Subnet::get(id))[0];
  const auto& subnet = Subnet::get(id);
  const auto config = kitty::exact_npn_canonization(tt);
  TT canonTT = eda::utils::getTT(config);
  NPNTransformation t = eda::utils::getTransformation(config);
  auto& newSubnet = Subnet::get(npnTransform(subnet, t));
  tt = evaluate(newSubnet)[0];
  return tt == canonTT;
}

static bool npnDatabaseTest(const SubnetID &id) {
  NPNDatabase2 npndb;
  const auto& subnet = Subnet::get(id);
  npndb.push(id);
  uint16_t negationMask = 5;
  NPNTransformation::InputPermutation permutation;
  for (size_t i = 0; i < inputsCount(subnet); i++) {
    permutation.push_back(inputsCount(subnet) - i - 1);
  }
  NPNTransformation t = {negationMask, permutation};
  auto& subnet1 = Subnet::get(npnTransform(subnet, t));
  auto res = npndb.get(subnet1);
  const auto& subnet2 = Subnet::get(res.get());

  return areEquivalent(subnet1, subnet2);
}

TEST(NPNDB2, TransformTest) {
  EXPECT_TRUE(transformTest(makeSubnet3AndOrXor()));
  EXPECT_TRUE(transformTest(makeSubnetXorNorAndAndOr()));
  EXPECT_TRUE(transformTest(makeSubnetXorOrXor()));
}

TEST(NPNDB2, npnDatabaseTest) {
  EXPECT_TRUE(npnDatabaseTest(makeSubnet3AndOrXor()));
  EXPECT_TRUE(npnDatabaseTest(makeSubnetXorNorAndAndOr()));
  EXPECT_TRUE(npnDatabaseTest(makeSubnetXorOrXor()));
}
