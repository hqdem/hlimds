//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/subnet.h"
#include "gate/model/utils/subnet_to_bdd.h"
#include "util/logging.h"

#include "gtest/gtest.h"

using namespace eda::gate::model;
using namespace eda::gate::model::utils;

BDD handle(SubnetBuilder &subnetBuilder, Cudd &manager, CellBDDMap &x) {
  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  for (int i = 0; i < subnet.getInNum(); i++) {
    x[i] = manager.bddVar(i);
  }
  // Output gate is the last element in entries array
  unsigned ouputId = subnet.size() - 1;
  return SubnetToBdd::convert(subnet, ouputId, manager);
}

TEST(SubnetToBddTest, ZeroTest) {
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[3];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(ZERO, idx[0]);
  subnetBuilder.addCell(OUT, idx[2]);

  Cudd manager(0, 0);
  CellBDDMap x;

  BDD test = manager.bddZero();
  EXPECT_TRUE(handle(subnetBuilder, manager, x) == test);
}

TEST(SubnetToBddTest, AndTest) {
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[3];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(AND, idx[0], idx[1]);
  subnetBuilder.addCell(OUT, idx[2]);

  Cudd manager(0, 0);
  CellBDDMap x;

  BDD result = handle(subnetBuilder, manager, x);
  BDD test = x[0] & x[1];
  EXPECT_TRUE(result == test);
}

TEST(SubnetToBddTest, AndOrTest) {
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[5];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(IN);
  idx[3] = subnetBuilder.addCell(AND, idx[0], idx[1]);
  idx[4] = subnetBuilder.addCell(OR, idx[2], idx[3]);
  subnetBuilder.addCell(OUT, idx[4]);

  Cudd manager(0, 0);
  CellBDDMap x;

  BDD result = handle(subnetBuilder, manager, x);
  BDD test = (x[0] & x[1]) | x[2];
  EXPECT_TRUE(result == test);
}

TEST(SubnetToBddTest, OrWithNotTest) {
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[3];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(OR, Subnet::Link(idx[0].idx, true), idx[1]);
  subnetBuilder.addCell(OUT, idx[2]);

  Cudd manager(0, 0);
  CellBDDMap x;

  BDD result = handle(subnetBuilder, manager, x);
  BDD test = (!x[0]) | x[1];
  EXPECT_TRUE(result == test);
}

TEST(SubnetToBddTest, OrWithDoubleNotTest) {
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[3];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(OR, Subnet::Link(idx[0].idx, true), Subnet::Link(idx[1].idx, true));
  subnetBuilder.addCell(OUT, idx[2]);

  Cudd manager(0, 0);
  CellBDDMap x;

  BDD result = handle(subnetBuilder, manager, x);
  BDD test = (!x[0]) | (!x[1]);
  EXPECT_TRUE(result == test);
}

TEST(SubnetToBddTest, ITETest) {
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[40];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(IN);
  idx[12] = subnetBuilder.addCell(AND, idx[0], idx[1]);
  idx[13] = subnetBuilder.addCell(AND, Subnet::Link(idx[0].idx, true), idx[2]);
  idx[14] = subnetBuilder.addCell(OR, idx[12], idx[13]);
  subnetBuilder.addCell(OUT, idx[14]);

  Cudd manager(0, 0);
  CellBDDMap x;

  BDD result = handle(subnetBuilder, manager, x);
  BDD test = (x[0] & x[1]) | ((!x[0]) & x[2]);
  EXPECT_TRUE(result == test);
}

TEST(SubnetToBddTest, ITETest2) {
  // ITE([(!a+b)^c],c,d)]
  /* Subnet creation */
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[40];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(IN);
  idx[3] = subnetBuilder.addCell(IN);
  idx[12] = subnetBuilder.addCell(OR,
                                  Subnet::Link(idx[0].idx, true),
                                  idx[1]); // !a+b
  idx[13] = subnetBuilder.addCell(XOR, idx[12], idx[2]); // (!a+b)^c
  idx[14] = subnetBuilder.addCell(AND, idx[13], idx[2]); // ((!a+b)^c)*c
  idx[16] = subnetBuilder.addCell(AND,
                                  Subnet::Link(idx[13].idx, true),
                                  idx[3]); // !((!a+b)^c)*d
  idx[17] = subnetBuilder.addCell(OR, idx[14], idx[16]); // ITE(idx[13],c,d)
  subnetBuilder.addCell(OUT, idx[17]);

  Cudd manager(0, 0);
  CellBDDMap x;

  BDD result = handle(subnetBuilder, manager, x);
  BDD test = (((( (!x[0]) | x[1]) ^ x[2]) & x[2]) |
      ((!( (!x[0]) | x[1]) ^ x[2]) & x[3]));
  EXPECT_TRUE(result == test);
}
