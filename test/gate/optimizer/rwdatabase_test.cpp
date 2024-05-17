//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gtest/gtest.h"

#include "gate/optimizer/rwdatabase.h"
#include "gate/model/examples.h"
#include "gate/model/utils/subnet_truth_table.h"

using namespace eda::gate::model;
using namespace eda::gate::optimizer;

static bool areEquivalent(const Subnet &lhs,
                          const Subnet &rhs) {
  return evaluate(lhs) == evaluate(rhs);
}

TEST(RWDatabase2Test, BasicTest) {
  RWDatabase2 rwdb;

  SubnetID id = makeSubnet3AndOrXor();
  RWDatabase2::TT tt = evaluate(Subnet::get(id))[0];

  rwdb.push(tt, id);
  EXPECT_TRUE(areEquivalent(Subnet::get(rwdb.get(tt)[0]), Subnet::get(id)));

  EXPECT_FALSE(rwdb.empty());
  rwdb.erase(tt);
  EXPECT_TRUE(rwdb.empty());
}
