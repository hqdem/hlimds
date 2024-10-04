//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/subnetview.h"
#include "gate/model/utils/subnetview_to_bdd.h"

#include "gtest/gtest.h"

using namespace eda::gate::model;
using namespace eda::gate::model::utils;

BDD addInputsAndConvert(SubnetBuilder &subnetBuilder, Cudd &manager, BddMap &x) {
  for (int i = 0; i < subnetBuilder.getInNum(); i++) {
    x[i] = manager.bddVar(i);
  }
  SubnetView sv(subnetBuilder);
  return convertBdd(sv, manager).at(0);
}

TEST(SubnetViewToBddTest, ZeroTest) {
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[1];
  idx[0] = subnetBuilder.addCell(ZERO);
  subnetBuilder.addCell(OUT, idx[0]);

  Cudd manager(0, 0);
  BddMap x;

  BDD result = addInputsAndConvert(subnetBuilder, manager, x);
  BDD test = manager.bddZero();
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, OneTest) {
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[1];
  idx[0] = subnetBuilder.addCell(ONE);
  subnetBuilder.addCell(OUT, idx[0]);

  Cudd manager(0, 0);
  BddMap x;

  BDD result = addInputsAndConvert(subnetBuilder, manager, x);
  BDD test = manager.bddOne();
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, NotTest) {
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[1];
  idx[0] = subnetBuilder.addCell(IN);
  subnetBuilder.addCell(OUT, Subnet::Link(idx[0].idx, true));

  Cudd manager(0, 0);
  BddMap x;

  BDD result = addInputsAndConvert(subnetBuilder, manager, x);
  BDD test = !x[0];
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, BufTest) {
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[2];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(BUF, idx[0]);
  subnetBuilder.addCell(OUT, idx[1]);

  Cudd manager(0, 0);
  BddMap x;

  BDD result = addInputsAndConvert(subnetBuilder, manager, x);
  BDD test = x[0];
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, AndTest) {
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[3];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(AND, idx[0], idx[1]);
  subnetBuilder.addCell(OUT, idx[2]);

  Cudd manager(0, 0);
  BddMap x;

  BDD result = addInputsAndConvert(subnetBuilder, manager, x);
  BDD test = x[0] & x[1];
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, OrTest) {
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[3];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(OR, idx[0], idx[1]);
  subnetBuilder.addCell(OUT, idx[2]);

  Cudd manager(0, 0);
  BddMap x;

  BDD result = addInputsAndConvert(subnetBuilder, manager, x);
  BDD test = x[0] | x[1];
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, XorTest) {
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[3];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(XOR, idx[0], idx[1]);
  subnetBuilder.addCell(OUT, idx[2]);

  Cudd manager(0, 0);
  BddMap x;

  BDD result = addInputsAndConvert(subnetBuilder, manager, x);
  BDD test = x[0] ^ x[1];
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, NandTest) {
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[3];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(AND, idx[0], idx[1]);
  subnetBuilder.addCell(OUT, Subnet::Link(idx[2].idx, true));

  Cudd manager(0, 0);
  BddMap x;

  BDD result = addInputsAndConvert(subnetBuilder, manager, x);
  BDD test = !(x[0] & x[1]);
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, NorTest) {
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[3];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(OR, idx[0], idx[1]);
  subnetBuilder.addCell(OUT, Subnet::Link(idx[2].idx, true));

  Cudd manager(0, 0);
  BddMap x;

  BDD result = addInputsAndConvert(subnetBuilder, manager, x);
  BDD test = !(x[0] | x[1]);
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, XnorTest) {
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[3];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(XOR, idx[0], idx[1]);
  subnetBuilder.addCell(OUT, Subnet::Link(idx[2].idx, true));

  Cudd manager(0, 0);
  BddMap x;

  BDD result = addInputsAndConvert(subnetBuilder, manager, x);
  BDD test = !(x[0] ^ x[1]);
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, MajTest) {
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[3];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(IN);
  idx[3] = subnetBuilder.addCell(MAJ, idx[0], idx[1], idx[2]);
  subnetBuilder.addCell(OUT, idx[3]);

  Cudd manager(0, 0);
  BddMap x;

  BDD result = addInputsAndConvert(subnetBuilder, manager, x);
  BDD test = (x[0] & x[1]) | (x[0] & x[2]) | (x[1] & x[2]);
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, AndOrTest) {
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[5];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(IN);
  idx[3] = subnetBuilder.addCell(AND, idx[0], idx[1]);
  idx[4] = subnetBuilder.addCell(OR, idx[2], idx[3]);
  subnetBuilder.addCell(OUT, idx[4]);

  Cudd manager(0, 0);
  BddMap x;

  BDD result = addInputsAndConvert(subnetBuilder, manager, x);
  BDD test = (x[0] & x[1]) | x[2];
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, OrWithNotTest) {
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[3];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(OR, Subnet::Link(idx[0].idx, true), idx[1]);
  subnetBuilder.addCell(OUT, idx[2]);

  Cudd manager(0, 0);
  BddMap x;

  BDD result = addInputsAndConvert(subnetBuilder, manager, x);
  BDD test = (!x[0]) | x[1];
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, OrWithDoubleNotTest) {
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[3];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(OR, Subnet::Link(idx[0].idx, true), Subnet::Link(idx[1].idx, true));
  subnetBuilder.addCell(OUT, idx[2]);

  Cudd manager(0, 0);
  BddMap x;

  BDD result = addInputsAndConvert(subnetBuilder, manager, x);
  BDD test = (!x[0]) | (!x[1]);
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, ITETest) {
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
  BddMap x;

  BDD result = addInputsAndConvert(subnetBuilder, manager, x);
  BDD test = (x[0] & x[1]) | ((!x[0]) & x[2]);
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, ITETest2) {
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
  BddMap x;

  BDD result = addInputsAndConvert(subnetBuilder, manager, x);
  BDD test = (((( (!x[0]) | x[1]) ^ x[2]) & x[2]) |
      ((!( (!x[0]) | x[1]) ^ x[2]) & x[3]));
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, ForestAndTest) {
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[5];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(IN);
  idx[3] = subnetBuilder.addCell(AND, idx[0], idx[1]);
  idx[4] = subnetBuilder.addCell(AND, idx[1], idx[2]);
  subnetBuilder.addCell(OUT, idx[3]);
  subnetBuilder.addCell(OUT, idx[4]);

  Cudd manager(0, 0);
  BddMap x;

  for (int i = 0; i < subnetBuilder.getInNum(); i++) {
    x[i] = manager.bddVar(i);
  }

  BDD test = x[0] & x[1];
  BDD test1 = x[1] & x[2];

  SubnetView sv(subnetBuilder);
  auto BDDForest = convertBdd(sv, manager);
  EXPECT_TRUE(BDDForest.at(0) == test && BDDForest.at(1) == test1);
}

TEST(SubnetViewToBddTest, nineAndTest) {
  SubnetBuilder subnetBuilder;
  Subnet::LinkList list;
  for (int i = 0; i < 9; ++i) {
    list.push_back(subnetBuilder.addCell(IN));
  }
  Subnet::Link out = subnetBuilder.addCell(AND, list);
  subnetBuilder.addCell(OUT, out);

  Cudd manager(0, 0);
  BddMap x;

  for (int i = 0; i < subnetBuilder.getInNum(); i++) {
    x[i] = manager.bddVar(i);
  }

  SubnetView sv(subnetBuilder);
  BDD test = x[0] & x[1] & x[2] & x[3] & x[4] & x[5] & x[6] & x[7] & x[8];
  BDD result = convertBdd(sv, manager).at(0);
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, notWholeNetTest) {
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[12];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(IN);
  idx[3] = subnetBuilder.addCell(IN);
  idx[4] = subnetBuilder.addCell(IN);
  idx[5] = subnetBuilder.addCell(IN);
  idx[6] = subnetBuilder.addCell(AND, idx[0], idx[1]);
  idx[7] = subnetBuilder.addCell(AND, idx[2], idx[3]);
  idx[8] = subnetBuilder.addCell(AND, idx[4], idx[5]);
  idx[9] = subnetBuilder.addCell(AND, idx[6], idx[7]);
  idx[10] = subnetBuilder.addCell(AND, idx[7], idx[8]);
  idx[11] = subnetBuilder.addCell(AND, idx[9], idx[10]);
  subnetBuilder.addCell(OUT, idx[11]);

  Cudd manager(0, 0);
  BddMap x;

  x[0] = manager.bddVar(6);
  x[1] = manager.bddVar(7);
  x[2] = manager.bddVar(8);

  BDD test = (x[0] & x[1]) & (x[1] & x[2]);

  InOutMapping inout({6,7,8}, {11});
  SubnetView sv(subnetBuilder, inout);
  auto BDDForest = convertBdd(sv, manager);
  EXPECT_TRUE(BDDForest.at(0) == test);
}
