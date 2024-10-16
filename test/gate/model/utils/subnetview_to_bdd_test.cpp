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

using BuilderPtr = std::shared_ptr<SubnetBuilder>;

BDD addInputsAndConvert(BuilderPtr &builderPtr, Cudd &manager, BddMap &x) {
  for (size_t i = 0; i < builderPtr->getInNum(); i++) {
    x[i] = manager.bddVar(i);
  }
  SubnetView sv(builderPtr);
  return convertBdd(sv, manager).at(0);
}

TEST(SubnetViewToBddTest, ZeroTest) {
  auto builderPtr = std::make_shared<SubnetBuilder>();
  Subnet::Link idx[1];
  idx[0] = builderPtr->addCell(ZERO);
  builderPtr->addCell(OUT, idx[0]);

  Cudd manager(0, 0);
  BddMap x;

  BDD result = addInputsAndConvert(builderPtr, manager, x);
  BDD test = manager.bddZero();
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, OneTest) {
  auto builderPtr = std::make_shared<SubnetBuilder>();
  Subnet::Link idx[1];
  idx[0] = builderPtr->addCell(ONE);
  builderPtr->addCell(OUT, idx[0]);

  Cudd manager(0, 0);
  BddMap x;

  BDD result = addInputsAndConvert(builderPtr, manager, x);
  BDD test = manager.bddOne();
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, NotTest) {
  auto builderPtr = std::make_shared<SubnetBuilder>();
  Subnet::Link idx[1];
  idx[0] = builderPtr->addCell(IN);
  builderPtr->addCell(OUT, Subnet::Link(idx[0].idx, true));

  Cudd manager(0, 0);
  BddMap x;

  BDD result = addInputsAndConvert(builderPtr, manager, x);
  BDD test = !x[0];
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, BufTest) {
  auto builderPtr = std::make_shared<SubnetBuilder>();
  Subnet::Link idx[2];
  idx[0] = builderPtr->addCell(IN);
  idx[1] = builderPtr->addCell(BUF, idx[0]);
  builderPtr->addCell(OUT, idx[1]);

  Cudd manager(0, 0);
  BddMap x;

  BDD result = addInputsAndConvert(builderPtr, manager, x);
  BDD test = x[0];
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, AndTest) {
  auto builderPtr = std::make_shared<SubnetBuilder>();
  Subnet::Link idx[3];
  idx[0] = builderPtr->addCell(IN);
  idx[1] = builderPtr->addCell(IN);
  idx[2] = builderPtr->addCell(AND, idx[0], idx[1]);
  builderPtr->addCell(OUT, idx[2]);

  Cudd manager(0, 0);
  BddMap x;

  BDD result = addInputsAndConvert(builderPtr, manager, x);
  BDD test = x[0] & x[1];
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, OrTest) {
  auto builderPtr = std::make_shared<SubnetBuilder>();
  Subnet::Link idx[3];
  idx[0] = builderPtr->addCell(IN);
  idx[1] = builderPtr->addCell(IN);
  idx[2] = builderPtr->addCell(OR, idx[0], idx[1]);
  builderPtr->addCell(OUT, idx[2]);

  Cudd manager(0, 0);
  BddMap x;

  BDD result = addInputsAndConvert(builderPtr, manager, x);
  BDD test = x[0] | x[1];
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, XorTest) {
  auto builderPtr = std::make_shared<SubnetBuilder>();
  Subnet::Link idx[3];
  idx[0] = builderPtr->addCell(IN);
  idx[1] = builderPtr->addCell(IN);
  idx[2] = builderPtr->addCell(XOR, idx[0], idx[1]);
  builderPtr->addCell(OUT, idx[2]);

  Cudd manager(0, 0);
  BddMap x;

  BDD result = addInputsAndConvert(builderPtr, manager, x);
  BDD test = x[0] ^ x[1];
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, NandTest) {
  auto builderPtr = std::make_shared<SubnetBuilder>();
  Subnet::Link idx[3];
  idx[0] = builderPtr->addCell(IN);
  idx[1] = builderPtr->addCell(IN);
  idx[2] = builderPtr->addCell(AND, idx[0], idx[1]);
  builderPtr->addCell(OUT, Subnet::Link(idx[2].idx, true));

  Cudd manager(0, 0);
  BddMap x;

  BDD result = addInputsAndConvert(builderPtr, manager, x);
  BDD test = !(x[0] & x[1]);
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, NorTest) {
  auto builderPtr = std::make_shared<SubnetBuilder>();
  Subnet::Link idx[3];
  idx[0] = builderPtr->addCell(IN);
  idx[1] = builderPtr->addCell(IN);
  idx[2] = builderPtr->addCell(OR, idx[0], idx[1]);
  builderPtr->addCell(OUT, Subnet::Link(idx[2].idx, true));

  Cudd manager(0, 0);
  BddMap x;

  BDD result = addInputsAndConvert(builderPtr, manager, x);
  BDD test = !(x[0] | x[1]);
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, XnorTest) {
  auto builderPtr = std::make_shared<SubnetBuilder>();
  Subnet::Link idx[3];
  idx[0] = builderPtr->addCell(IN);
  idx[1] = builderPtr->addCell(IN);
  idx[2] = builderPtr->addCell(XOR, idx[0], idx[1]);
  builderPtr->addCell(OUT, Subnet::Link(idx[2].idx, true));

  Cudd manager(0, 0);
  BddMap x;

  BDD result = addInputsAndConvert(builderPtr, manager, x);
  BDD test = !(x[0] ^ x[1]);
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, MajTest) {
  auto builderPtr = std::make_shared<SubnetBuilder>();
  Subnet::Link idx[3];
  idx[0] = builderPtr->addCell(IN);
  idx[1] = builderPtr->addCell(IN);
  idx[2] = builderPtr->addCell(IN);
  idx[3] = builderPtr->addCell(MAJ, idx[0], idx[1], idx[2]);
  builderPtr->addCell(OUT, idx[3]);

  Cudd manager(0, 0);
  BddMap x;

  BDD result = addInputsAndConvert(builderPtr, manager, x);
  BDD test = (x[0] & x[1]) | (x[0] & x[2]) | (x[1] & x[2]);
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, AndOrTest) {
  auto builderPtr = std::make_shared<SubnetBuilder>();
  Subnet::Link idx[5];
  idx[0] = builderPtr->addCell(IN);
  idx[1] = builderPtr->addCell(IN);
  idx[2] = builderPtr->addCell(IN);
  idx[3] = builderPtr->addCell(AND, idx[0], idx[1]);
  idx[4] = builderPtr->addCell(OR, idx[2], idx[3]);
  builderPtr->addCell(OUT, idx[4]);

  Cudd manager(0, 0);
  BddMap x;

  BDD result = addInputsAndConvert(builderPtr, manager, x);
  BDD test = (x[0] & x[1]) | x[2];
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, OrWithNotTest) {
  auto builderPtr = std::make_shared<SubnetBuilder>();
  Subnet::Link idx[3];
  idx[0] = builderPtr->addCell(IN);
  idx[1] = builderPtr->addCell(IN);
  idx[2] = builderPtr->addCell(OR, Subnet::Link(idx[0].idx, true), idx[1]);
  builderPtr->addCell(OUT, idx[2]);

  Cudd manager(0, 0);
  BddMap x;

  BDD result = addInputsAndConvert(builderPtr, manager, x);
  BDD test = (!x[0]) | x[1];
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, OrWithDoubleNotTest) {
  auto builderPtr = std::make_shared<SubnetBuilder>();
  Subnet::Link idx[3];
  idx[0] = builderPtr->addCell(IN);
  idx[1] = builderPtr->addCell(IN);
  idx[2] = builderPtr->addCell(OR, Subnet::Link(idx[0].idx, true), Subnet::Link(idx[1].idx, true));
  builderPtr->addCell(OUT, idx[2]);

  Cudd manager(0, 0);
  BddMap x;

  BDD result = addInputsAndConvert(builderPtr, manager, x);
  BDD test = (!x[0]) | (!x[1]);
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, ITETest) {
  auto builderPtr = std::make_shared<SubnetBuilder>();
  Subnet::Link idx[40];
  idx[0] = builderPtr->addCell(IN);
  idx[1] = builderPtr->addCell(IN);
  idx[2] = builderPtr->addCell(IN);
  idx[12] = builderPtr->addCell(AND, idx[0], idx[1]);
  idx[13] = builderPtr->addCell(AND, Subnet::Link(idx[0].idx, true), idx[2]);
  idx[14] = builderPtr->addCell(OR, idx[12], idx[13]);
  builderPtr->addCell(OUT, idx[14]);

  Cudd manager(0, 0);
  BddMap x;

  BDD result = addInputsAndConvert(builderPtr, manager, x);
  BDD test = (x[0] & x[1]) | ((!x[0]) & x[2]);
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, ITETest2) {
  // ITE([(!a+b)^c],c,d)]
  /* Subnet creation */
  auto builderPtr = std::make_shared<SubnetBuilder>();
  Subnet::Link idx[40];
  idx[0] = builderPtr->addCell(IN);
  idx[1] = builderPtr->addCell(IN);
  idx[2] = builderPtr->addCell(IN);
  idx[3] = builderPtr->addCell(IN);
  idx[12] = builderPtr->addCell(OR,
                                  Subnet::Link(idx[0].idx, true),
                                  idx[1]); // !a+b
  idx[13] = builderPtr->addCell(XOR, idx[12], idx[2]); // (!a+b)^c
  idx[14] = builderPtr->addCell(AND, idx[13], idx[2]); // ((!a+b)^c)*c
  idx[16] = builderPtr->addCell(AND,
                                  Subnet::Link(idx[13].idx, true),
                                  idx[3]); // !((!a+b)^c)*d
  idx[17] = builderPtr->addCell(OR, idx[14], idx[16]); // ITE(idx[13],c,d)
  builderPtr->addCell(OUT, idx[17]);

  Cudd manager(0, 0);
  BddMap x;

  BDD result = addInputsAndConvert(builderPtr, manager, x);
  BDD test = (((( (!x[0]) | x[1]) ^ x[2]) & x[2]) |
      ((!( (!x[0]) | x[1]) ^ x[2]) & x[3]));
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, ForestAndTest) {
  auto builderPtr = std::make_shared<SubnetBuilder>();
  Subnet::Link idx[5];
  idx[0] = builderPtr->addCell(IN);
  idx[1] = builderPtr->addCell(IN);
  idx[2] = builderPtr->addCell(IN);
  idx[3] = builderPtr->addCell(AND, idx[0], idx[1]);
  idx[4] = builderPtr->addCell(AND, idx[1], idx[2]);
  builderPtr->addCell(OUT, idx[3]);
  builderPtr->addCell(OUT, idx[4]);

  Cudd manager(0, 0);
  BddMap x;

  for (size_t i = 0; i < builderPtr->getInNum(); i++) {
    x[i] = manager.bddVar(i);
  }

  BDD test = x[0] & x[1];
  BDD test1 = x[1] & x[2];

  SubnetView sv(builderPtr);
  auto BDDForest = convertBdd(sv, manager);
  EXPECT_TRUE(BDDForest.at(0) == test && BDDForest.at(1) == test1);
}

TEST(SubnetViewToBddTest, nineAndTest) {
  auto builderPtr = std::make_shared<SubnetBuilder>();
  Subnet::LinkList list;
  for (size_t i = 0; i < 9; ++i) {
    list.push_back(builderPtr->addCell(IN));
  }
  Subnet::Link out = builderPtr->addCell(AND, list);
  builderPtr->addCell(OUT, out);

  Cudd manager(0, 0);
  BddMap x;

  for (size_t i = 0; i < builderPtr->getInNum(); i++) {
    x[i] = manager.bddVar(i);
  }

  SubnetView sv(builderPtr);
  BDD test = x[0] & x[1] & x[2] & x[3] & x[4] & x[5] & x[6] & x[7] & x[8];
  BDD result = convertBdd(sv, manager).at(0);
  EXPECT_TRUE(result == test);
}

TEST(SubnetViewToBddTest, notWholeNetTest) {
  auto builderPtr = std::make_shared<SubnetBuilder>();
  Subnet::Link idx[12];
  idx[0] = builderPtr->addCell(IN);
  idx[1] = builderPtr->addCell(IN);
  idx[2] = builderPtr->addCell(IN);
  idx[3] = builderPtr->addCell(IN);
  idx[4] = builderPtr->addCell(IN);
  idx[5] = builderPtr->addCell(IN);
  idx[6] = builderPtr->addCell(AND, idx[0], idx[1]);
  idx[7] = builderPtr->addCell(AND, idx[2], idx[3]);
  idx[8] = builderPtr->addCell(AND, idx[4], idx[5]);
  idx[9] = builderPtr->addCell(AND, idx[6], idx[7]);
  idx[10] = builderPtr->addCell(AND, idx[7], idx[8]);
  idx[11] = builderPtr->addCell(AND, idx[9], idx[10]);
  builderPtr->addCell(OUT, idx[11]);

  Cudd manager(0, 0);
  BddMap x;

  x[0] = manager.bddVar(6);
  x[1] = manager.bddVar(7);
  x[2] = manager.bddVar(8);

  BDD test = (x[0] & x[1]) & (x[1] & x[2]);

  InOutMapping inout({6,7,8}, {11});
  SubnetView sv(builderPtr, inout);
  auto BDDForest = convertBdd(sv, manager);
  EXPECT_TRUE(BDDForest.at(0) == test);
}
