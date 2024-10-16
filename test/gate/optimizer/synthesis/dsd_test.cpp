//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/subnet.h"
#include "gate/model/subnetview.h"
#include "gate/model/utils/subnet_random.h"
#include "gate/model/utils/subnet_random.h"
#include "gate/model/utils/subnet_truth_table.h"
#include "gate/model/utils/subnetview_to_bdd.h"
#include "gate/optimizer/synthesis/dsd_to_subnet.h"
#include "gate/optimizer/synthesis/isop.h"
#include "gate/simulator/simulator.h"
#include "gate/optimizer/resynthesizer.h"

#include "gtest/gtest.h"

using namespace eda::gate::model;
using namespace eda::gate::model::utils;
using namespace eda::gate::simulator;

using DsdSynthesizer = eda::gate::optimizer::synthesis::DsdSynthesizer;
using BddWithDdManager = eda::gate::optimizer::synthesis::BddWithDdManager;
using LinkList = Subnet::LinkList;

Subnet::Link addITE(Subnet::Link first,
                    Subnet::Link second,
                    Subnet::Link third,
                    SubnetBuilder &subnetBuilder,
                    bool sign = 0) {
  Subnet::Link idx[2];
  idx[0] = subnetBuilder.addCell(AND, first, second);
  idx[1] = subnetBuilder.addCell(AND,
                                 Subnet::Link(first.idx, !first.inv),
                                 third);
  return Subnet::Link(subnetBuilder.addCell(OR, idx[0], idx[1]).idx, sign);
}

const Subnet& handle(const Subnet &subnet) {
  auto builder = std::make_shared<SubnetBuilder>(subnet);
  eda::gate::model::SubnetView sv(builder);

  /* Resynthesis */
  DsdSynthesizer dsdSynthesizer;
  eda::gate::optimizer::Resynthesizer<Bdd> res(dsdSynthesizer);
  const auto &result = res.resynthesize(sv).makeObject();
  LOG_DEBUG(result);

  return result;
}

/* Variables in comments correspond to numbers in alphabet order
 * Cudd_bddIthVar(manager, 0) => a
 * Cudd_bddIthVar(manager, 1) => b and etc. */

TEST(DsdTest, negationOnTruthPath) {
  // ITE(c,(a + b),d)
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[6];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[2] = subnetBuilder.addCell(IN); //c
  idx[3] = subnetBuilder.addCell(IN); //d
  idx[4] = subnetBuilder.addCell(AND, idx[0], idx[1]);
  idx[5] = addITE(idx[2], idx[4], idx[3], subnetBuilder);
  subnetBuilder.addCell(OUT, idx[5]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, complementationTopNode) {
  // a * ((b XOR c) + d)
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[7];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[2] = subnetBuilder.addCell(IN); //c
  idx[3] = subnetBuilder.addCell(IN); //d
  idx[4] = subnetBuilder.addCell(XOR, idx[1], idx[2]);
  idx[5] = subnetBuilder.addCell(OR, idx[4], idx[3]);
  idx[6] = subnetBuilder.addCell(AND, idx[0], idx[5]);
  subnetBuilder.addCell(OUT, idx[6]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, PrimeInPrime) {
  // ITE(a,b,ITE(c,d,e))
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[7];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[2] = subnetBuilder.addCell(IN); //c
  idx[3] = subnetBuilder.addCell(IN); //d
  idx[4] = subnetBuilder.addCell(IN); //e
  idx[5] = addITE(idx[2], idx[3], idx[4], subnetBuilder);
  idx[6] = addITE(idx[0], idx[1], idx[5], subnetBuilder);
  subnetBuilder.addCell(OUT, idx[6]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, SimpleMixFunctions) {
  // ITE((a + b),c,d)
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[7];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[2] = subnetBuilder.addCell(IN); //c
  idx[3] = subnetBuilder.addCell(IN); //d
  idx[4] = subnetBuilder.addCell(AND, idx[0], idx[1]);
  idx[5] = addITE(idx[4], idx[2], idx[3], subnetBuilder);
  subnetBuilder.addCell(OUT, idx[5]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, MixFunctions) {
  // ITE(a, e, (!b xor !c) + d)
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[8];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[2] = subnetBuilder.addCell(IN); //c
  idx[3] = subnetBuilder.addCell(IN); //d
  idx[4] = subnetBuilder.addCell(IN); //e
  idx[5] = subnetBuilder.addCell(XOR,
                                 Subnet::Link(idx[1].idx, true),
                                 Subnet::Link(idx[2].idx, true));
  idx[6] = subnetBuilder.addCell(OR, idx[5], idx[3]);
  idx[7] = addITE(idx[0], idx[4], idx[6], subnetBuilder);
  subnetBuilder.addCell(OUT, idx[7]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, LowerPrimeFunction) {
  // (ITE(a,b,c)+d)
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[6];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[2] = subnetBuilder.addCell(IN); //c
  idx[3] = subnetBuilder.addCell(IN); //d
  idx[4] = addITE(idx[0], idx[1], idx[2], subnetBuilder);
  idx[5] = subnetBuilder.addCell(OR, idx[4], idx[3]);
  subnetBuilder.addCell(OUT, idx[5]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, LowerNotPrimeFunction) {
  // (NOT(ITE(a, b, c)) + d)
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[6];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[2] = subnetBuilder.addCell(IN); //c
  idx[3] = subnetBuilder.addCell(IN); //d
  idx[4] = addITE(idx[0], idx[1], idx[2], subnetBuilder);
  idx[5] = subnetBuilder.addCell(OR, Subnet::Link(idx[4].idx, true), idx[3]);
  subnetBuilder.addCell(OUT, idx[5]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, MajorityFunction) {
  // MAJ(a,b,c) = ab + ac + bc
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[6];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[2] = subnetBuilder.addCell(IN); //c
  idx[4] = subnetBuilder.addCell(MAJ, idx[0], idx[1], idx[2]);
  subnetBuilder.addCell(OUT, idx[4]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, fourAnd) {
  // AND(a,b,c,d)
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[6];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[2] = subnetBuilder.addCell(IN); //c
  idx[3] = subnetBuilder.addCell(IN); //c
  idx[4] = subnetBuilder.addCell(AND, idx[0], idx[1], idx[2], idx[3]);
  subnetBuilder.addCell(OUT, idx[4]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, nineAnd) {
  // AND(a,b,c,d,e,f,g,h,i)
  SubnetBuilder subnetBuilder;
  LinkList list;
  for (int i = 0; i < 9; ++i) {
    list.push_back(subnetBuilder.addCell(IN));
  }
  Subnet::Link out = subnetBuilder.addCell(AND, list);
  subnetBuilder.addCell(OUT, out);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, fourteenAnd) {
  // AND(a,b,c,d,e,f,g,h,i,j,k,l,m,n)
  SubnetBuilder subnetBuilder;
  LinkList list;
  for (int i = 0; i < 14; ++i) {
    list.push_back(subnetBuilder.addCell(IN));
  }
  Subnet::Link out = subnetBuilder.addCell(AND, list);
  subnetBuilder.addCell(OUT, out);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, tenXor) {
  // XOR(a,b,c,d,e,f,g,h,i,j)
  SubnetBuilder subnetBuilder;
  LinkList list;
  for (int i = 0; i < 10; ++i) {
    list.push_back(subnetBuilder.addCell(IN));
  }
  Subnet::Link out = subnetBuilder.addCell(XOR, list);
  subnetBuilder.addCell(OUT, out);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, elevenOr) {
  // OR(a,b,c,d,e,f,g,h,i,j,k)
  SubnetBuilder subnetBuilder;
  LinkList list;
  for (int i = 0; i < 11; ++i) {
    list.push_back(subnetBuilder.addCell(IN));
  }
  Subnet::Link out = subnetBuilder.addCell(OR, list);
  subnetBuilder.addCell(OUT, out);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, ManyNodesTest) {
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[21];
  for (int i = 0; i < 10; ++i) {
    idx[i] = subnetBuilder.addCell(IN);
  }
  idx[11] = addITE(idx[0], idx[1], idx[2], subnetBuilder);
  idx[12] = addITE(idx[11], idx[3], idx[4], subnetBuilder);
  idx[13] = subnetBuilder.addCell(OR, idx[12], idx[3]);
  idx[14] = subnetBuilder.addCell(AND, idx[13], idx[4]);
  idx[15] = subnetBuilder.addCell(AND, idx[14], idx[5]);
  idx[16] = subnetBuilder.addCell(AND, idx[15], idx[6]);
  idx[17] = subnetBuilder.addCell(XOR, Subnet::Link(idx[16].idx, true), idx[7]);
  idx[18] = subnetBuilder.addCell(XOR, Subnet::Link(idx[17].idx, true), idx[8]);
  idx[19] = subnetBuilder.addCell(XOR, Subnet::Link(idx[18].idx, true), idx[9]);
  idx[20] = addITE(idx[19], idx[18], idx[15], subnetBuilder);
  subnetBuilder.addCell(OUT, idx[20]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, DsdNegationTest) {
  // ITE(b,c,d)*a

  SubnetBuilder subnetBuilder;
  Subnet::Link idx[6];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(IN);
  idx[3] = subnetBuilder.addCell(IN);
  idx[4] = addITE(idx[1], idx[2], idx[3], subnetBuilder);
  idx[5] = subnetBuilder.addCell(AND, idx[4], idx[0]);
  subnetBuilder.addCell(OUT, idx[5]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, PointerNegationTest) {
  // ITE(a,b,c)*d

  SubnetBuilder subnetBuilder;
  Subnet::Link idx[6];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(IN);
  idx[3] = subnetBuilder.addCell(IN);
  idx[4] = addITE(idx[0], idx[1], idx[2], subnetBuilder);
  idx[5] = subnetBuilder.addCell(AND, idx[4], idx[3]);
  subnetBuilder.addCell(OUT, idx[5]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, DsdWithoutNegationTest) {
  // ITE(b,c,d)+a

  SubnetBuilder subnetBuilder;
  Subnet::Link idx[6];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(IN);
  idx[3] = subnetBuilder.addCell(IN);
  idx[4] = addITE(idx[1], idx[2], idx[3], subnetBuilder);
  idx[5] = subnetBuilder.addCell(OR, idx[4], idx[0]);
  subnetBuilder.addCell(OUT, idx[5]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, SubnetAndTest) {
  /* Subnet creation */
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[3];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(AND, idx[0], idx[1]);
  subnetBuilder.addCell(OUT, Subnet::Link(idx[2]));

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, SubnetFourVarsTest) {
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

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, AndXor) {
  /* Subnet creation */
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[40];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(IN);
  idx[3] = subnetBuilder.addCell(IN);
  idx[12] = subnetBuilder.addCell(AND, Subnet::Link(idx[0].idx, true), idx[1]); // !a+b
  idx[13] = subnetBuilder.addCell(XOR, idx[12], idx[2]); // (!a+b)^c
  idx[14] = subnetBuilder.addCell(AND, idx[13], idx[3]); // ((!a+b)^c)*d
  subnetBuilder.addCell(OUT, Subnet::Link(idx[14]));

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, AndXorXnor) {
  // ITE([(!a+b)^c],c,d)]
  /* Subnet creation */
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[40];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(IN);
  idx[3] = subnetBuilder.addCell(IN);
  idx[4] = subnetBuilder.addCell(IN);
  idx[12] = subnetBuilder.addCell(AND, Subnet::Link(idx[0].idx, true), idx[1]); // !a+b
  idx[13] = subnetBuilder.addCell(XOR, idx[12], idx[2]); // (!a+b)^c
  idx[14] = subnetBuilder.addCell(AND, idx[13], idx[3]); // ((!a+b)^c)*d
  idx[15] = subnetBuilder.addCell(XOR, idx[14], idx[4]); // !(((!a+b)^c)*d^e)
  subnetBuilder.addCell(OUT, Subnet::Link(idx[15].idx, true));

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, MultiAnd) {
  // ITE([(!a+b)^c],c,d)]
  /* Subnet creation */
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[40];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(IN);
  idx[3] = subnetBuilder.addCell(IN);
  idx[4] = subnetBuilder.addCell(IN);
  idx[12] = subnetBuilder.addCell(OR, Subnet::Link(idx[0].idx, true), idx[1]); // !a+b
  idx[13] = subnetBuilder.addCell(XOR, idx[12], idx[2]); // (!a+b)^c
  idx[14] = subnetBuilder.addCell(AND, idx[12], idx[2]); // ((!a+b)^c)*c
  idx[16] = subnetBuilder.addCell(AND, Subnet::Link(idx[12].idx, true), idx[3]); // !((!a+b)^c)*d
  idx[17] = subnetBuilder.addCell(OR, idx[14], idx[16]); // ITE(idx[13],c,d)
  idx[18] = subnetBuilder.addCell(XOR, idx[17], idx[4]); //
  idx[19] = subnetBuilder.addCell(AND, idx[18], idx[4]); //
  subnetBuilder.addCell(OUT, Subnet::Link(idx[19]));

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, NorTest) {
  /* Subnet creation */
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[40];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(IN);
  idx[3] = subnetBuilder.addCell(IN);
  idx[4] = subnetBuilder.addCell(IN);
  idx[12] = subnetBuilder.addCell(OR, idx[0], idx[1], idx[2], idx[3], idx[4]);
  subnetBuilder.addCell(OUT, Subnet::Link(idx[12].idx, true));

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, XorTest) {
  /* Subnet creation */
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[40];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(IN);
  idx[3] = subnetBuilder.addCell(IN);
  idx[4] = subnetBuilder.addCell(IN);
  idx[12] = subnetBuilder.addCell(XOR, idx[0], idx[1], idx[2], idx[3], idx[4]);
  subnetBuilder.addCell(OUT, Subnet::Link(idx[12]));
  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, SubnetITETest) {
  // ITE([a,b,c)]
  /* Subnet creation */
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[40];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(IN);
  idx[12] = subnetBuilder.addCell(AND, idx[0], idx[1]);
  idx[13] = subnetBuilder.addCell(AND, Subnet::Link(idx[0].idx, true), idx[2]);
  idx[14] = subnetBuilder.addCell(OR, idx[12], idx[13]);
  subnetBuilder.addCell(OUT, idx[14]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, SubnetNegativeITETest) {
  // NOT(ITE(a,b,c))
  /* Subnet creation */
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[40];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(IN);
  idx[12] = addITE(idx[0], idx[1], idx[2], subnetBuilder);
  subnetBuilder.addCell(OUT, Subnet::Link(idx[12].idx, true));

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, SubnetTenVarsTest) {
  // ITE(![!(g*h)^i],j,!(ITE[e,f,ITE([(!a+b)^c],c,d)]+i))
  /* Subnet creation */
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[40];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[2] = subnetBuilder.addCell(IN); //c
  idx[3] = subnetBuilder.addCell(IN); //d
  idx[4] = subnetBuilder.addCell(IN); //e
  idx[5] = subnetBuilder.addCell(IN); //f
  idx[6] = subnetBuilder.addCell(IN); //g
  idx[7] = subnetBuilder.addCell(IN); //h
  idx[8] = subnetBuilder.addCell(IN); //i
  idx[9] = subnetBuilder.addCell(IN); //j
  idx[12] = subnetBuilder.addCell(OR, Subnet::Link(idx[0].idx, true), idx[1]);
  idx[13] = subnetBuilder.addCell(XOR, idx[12], idx[2]);
  idx[14] = addITE(idx[13], idx[2], idx[3], subnetBuilder);
  idx[15] = addITE(idx[4], idx[5], idx[14], subnetBuilder);
  idx[16] = subnetBuilder.addCell(AND, idx[6], idx[7]);
  idx[17] = subnetBuilder.addCell(XOR, idx[8], Subnet::Link(idx[16].idx, true));
  idx[18] = subnetBuilder.addCell(OR, idx[15], idx[8]);
  idx[19] = addITE(Subnet::Link(idx[17].idx, true), idx[9], Subnet::Link(idx[18].idx, true), subnetBuilder);
  subnetBuilder.addCell(OUT, idx[19]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, SixVarsTest) {
  // !(ITE[e,f,ITE([(!a+b)^c],c,d)]+g)
  /* Subnet creation */
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[40];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[2] = subnetBuilder.addCell(IN); //c
  idx[3] = subnetBuilder.addCell(IN); //d
  idx[4] = subnetBuilder.addCell(IN); //e
  idx[5] = subnetBuilder.addCell(IN); //f
  idx[6] = subnetBuilder.addCell(IN); //g
  idx[12] = subnetBuilder.addCell(OR, Subnet::Link(idx[0].idx, true), idx[1]);
  idx[13] = subnetBuilder.addCell(XOR, idx[12], idx[2]);
  idx[14] = addITE(idx[13], idx[2], idx[3], subnetBuilder);
  idx[15] = addITE(idx[4], idx[5], idx[14], subnetBuilder);
  idx[16] = subnetBuilder.addCell(OR, idx[15], idx[6]);
  subnetBuilder.addCell(OUT, Subnet::Link(idx[16].idx, true));

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, PrimePrimePrime) {
  // ITE(a,b,ITE(c,d,ITE(e,f,g)))
  /* Subnet creation */
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[40];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[2] = subnetBuilder.addCell(IN); //c
  idx[3] = subnetBuilder.addCell(IN); //d
  idx[4] = subnetBuilder.addCell(IN); //e
  idx[5] = subnetBuilder.addCell(IN); //f
  idx[6] = subnetBuilder.addCell(IN); //g
  idx[12] = addITE(idx[4], idx[5], idx[6], subnetBuilder);
  idx[13] = addITE(idx[2], idx[3], idx[12], subnetBuilder);
  idx[14] = addITE(idx[0], idx[1], idx[13], subnetBuilder);
  subnetBuilder.addCell(OUT, idx[14]);

  const auto &subnet = Subnet::get(subnetBuilder.make());  
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, PrimePrimePrimeWithH) {
  // ITE(a^h,b,ITE(c,d,ITE(e,f,g)+h))
  /* Subnet creation */
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[40];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[2] = subnetBuilder.addCell(IN); //c
  idx[3] = subnetBuilder.addCell(IN); //d
  idx[4] = subnetBuilder.addCell(IN); //e
  idx[5] = subnetBuilder.addCell(IN); //f
  idx[6] = subnetBuilder.addCell(IN); //g
  idx[7] = subnetBuilder.addCell(IN); //h
  idx[12] = addITE(idx[4], idx[5], idx[6], subnetBuilder);
  idx[13] = subnetBuilder.addCell(CellSymbol::OR, idx[12], idx[7]);
  idx[14] = addITE(idx[2], idx[3], idx[13], subnetBuilder);
  idx[15] = subnetBuilder.addCell(CellSymbol::XOR, idx[0], idx[7]);
  idx[16] = addITE(idx[15], idx[1], idx[14], subnetBuilder);
  subnetBuilder.addCell(OUT, idx[16]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, PrimePrimePrimeWithHwithNegation) {
  // ITE(a^h,b,!(ITE(c,d,ITE(e,f,g)+h)))
  /* Subnet creation */
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[40];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[2] = subnetBuilder.addCell(IN); //c
  idx[3] = subnetBuilder.addCell(IN); //d
  idx[4] = subnetBuilder.addCell(IN); //e
  idx[5] = subnetBuilder.addCell(IN); //f
  idx[6] = subnetBuilder.addCell(IN); //g
  idx[7] = subnetBuilder.addCell(IN); //h
  idx[12] = addITE(idx[4], idx[5], idx[6], subnetBuilder);
  idx[13] = subnetBuilder.addCell(CellSymbol::OR, idx[12], idx[7]);
  idx[14] = addITE(idx[2], idx[3], Subnet::Link(idx[13].idx, true), subnetBuilder);
  idx[15] = subnetBuilder.addCell(CellSymbol::XOR, idx[0], idx[7]);
  idx[16] = addITE(idx[15], idx[1], idx[14], subnetBuilder);
  subnetBuilder.addCell(OUT, idx[16]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, PrimePrimeWithNegation) {
  // ITE(a^f,b,!(ITE(c,d,e)+f))
  /* Subnet creation */
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[40];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[2] = subnetBuilder.addCell(IN); //c
  idx[3] = subnetBuilder.addCell(IN); //d
  idx[4] = subnetBuilder.addCell(IN); //e
  idx[5] = subnetBuilder.addCell(IN); //f
  idx[12] = addITE(idx[2], idx[3], idx[4], subnetBuilder);
  idx[13] = subnetBuilder.addCell(OR, idx[12], idx[5]);
  idx[14] = subnetBuilder.addCell(XOR, idx[0], idx[5]);
  idx[15] = addITE(idx[14], idx[1], Subnet::Link(idx[13].idx, true), subnetBuilder);
  subnetBuilder.addCell(OUT, idx[15]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, PrimePrimeWithNegation2) {
  // ITE(a,b,!(ITE(c,d,e)))
  /* Subnet creation */
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[40];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[2] = subnetBuilder.addCell(IN); //c
  idx[3] = subnetBuilder.addCell(IN); //d
  idx[4] = subnetBuilder.addCell(IN); //e
  idx[12] = addITE(idx[2], idx[3], idx[4], subnetBuilder);
  idx[13] = addITE(idx[0], idx[1], Subnet::Link(idx[12].idx, true), subnetBuilder);
  subnetBuilder.addCell(OUT, idx[13]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, GatePyramid) {
  /* Subnet creation */
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[100];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(IN);
  idx[3] = subnetBuilder.addCell(IN);
  idx[4] = subnetBuilder.addCell(IN);

  // AND
  idx[5] = subnetBuilder.addCell(AND, idx[0], idx[1]);
  idx[6] = subnetBuilder.addCell(AND, idx[1], idx[2]);
  idx[7] = subnetBuilder.addCell(AND, idx[2], idx[3]);
  idx[8] = subnetBuilder.addCell(AND, idx[3], idx[4]);

  // OR
  idx[9] = subnetBuilder.addCell(OR, idx[5], idx[6]);
  idx[10] = subnetBuilder.addCell(OR, idx[6], idx[7]);
  idx[11] = subnetBuilder.addCell(OR, idx[7], idx[8]);

  // XOR
  idx[12] = subnetBuilder.addCell(XOR, idx[9], idx[10]);
  idx[13] = subnetBuilder.addCell(XOR, idx[10], idx[11]);

  // OR
  idx[14] = subnetBuilder.addCell(OR, idx[12], idx[13]);

  subnetBuilder.addCell(OUT, idx[14]);
  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, NegativeGatePyramid) {
  /* Subnet creation */
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[100];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(IN);
  idx[3] = subnetBuilder.addCell(IN);
  idx[4] = subnetBuilder.addCell(IN);

  // AND
  idx[5] = subnetBuilder.addCell(AND, Subnet::Link(idx[0].idx, true), Subnet::Link(idx[1].idx, true));
  idx[6] = subnetBuilder.addCell(AND, Subnet::Link(idx[1].idx, true), Subnet::Link(idx[2].idx, true));
  idx[7] = subnetBuilder.addCell(AND, Subnet::Link(idx[2].idx, true), Subnet::Link(idx[3].idx, true));
  idx[8] = subnetBuilder.addCell(AND, Subnet::Link(idx[3].idx, true), Subnet::Link(idx[4].idx, true));

  // OR
  idx[9] = subnetBuilder.addCell(OR, Subnet::Link(idx[5].idx, true), Subnet::Link(idx[6].idx, true));
  idx[10] = subnetBuilder.addCell(OR, Subnet::Link(idx[6].idx, true), Subnet::Link(idx[7].idx, true));
  idx[11] = subnetBuilder.addCell(OR, Subnet::Link(idx[7].idx, true), Subnet::Link(idx[8].idx, true));

  // XOR
  idx[12] = subnetBuilder.addCell(XOR, Subnet::Link(idx[9].idx, true), Subnet::Link(idx[10].idx, true));
  idx[13] = subnetBuilder.addCell(XOR, Subnet::Link(idx[10].idx, true), Subnet::Link(idx[11].idx, true));

  // OR
  idx[14] = subnetBuilder.addCell(OR, Subnet::Link(idx[12].idx, true), Subnet::Link(idx[13].idx, true));

  subnetBuilder.addCell(OUT, Subnet::Link(idx[14].idx, true));

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, error) {
  /* Subnet creation */
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[100];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(IN);
  idx[2] = subnetBuilder.addCell(IN);
  idx[3] = subnetBuilder.addCell(IN);
  idx[4] = subnetBuilder.addCell(IN);

  // AND
  idx[5] = subnetBuilder.addCell(AND, Subnet::Link(idx[0].idx, true), Subnet::Link(idx[1].idx, true));
  idx[6] = subnetBuilder.addCell(AND, Subnet::Link(idx[1].idx, true), Subnet::Link(idx[2].idx, true));
  idx[7] = subnetBuilder.addCell(AND, Subnet::Link(idx[2].idx, true), Subnet::Link(idx[3].idx, true));
  idx[8] = subnetBuilder.addCell(AND, Subnet::Link(idx[3].idx, true), Subnet::Link(idx[4].idx, true));
  idx[9] = subnetBuilder.addCell(AND, Subnet::Link(idx[4].idx, true), idx[0]);

  // OR
  idx[10] = subnetBuilder.addCell(OR, idx[5], idx[6]);
  idx[11] = subnetBuilder.addCell(OR, idx[6], idx[7]);
  idx[12] = subnetBuilder.addCell(OR, idx[7], idx[8]);
  idx[13] = subnetBuilder.addCell(OR, idx[8], idx[9]);
  idx[14] = subnetBuilder.addCell(OR, idx[9], idx[5]);

  // XOR
  idx[15] = subnetBuilder.addCell(XOR, Subnet::Link(idx[10].idx, true), idx[11]);
  idx[16] = subnetBuilder.addCell(XOR, idx[11], idx[12]);
  idx[17] = subnetBuilder.addCell(XOR, idx[12], idx[13]);
  idx[18] = subnetBuilder.addCell(XOR, idx[13], idx[14]);
  idx[19] = subnetBuilder.addCell(XOR, idx[14], idx[10]);

  // AND
  idx[20] = subnetBuilder.addCell(AND, idx[15], idx[16]);
  idx[21] = subnetBuilder.addCell(AND, idx[16], idx[17]);
  idx[22] = subnetBuilder.addCell(AND, idx[17], idx[18]);
  idx[23] = subnetBuilder.addCell(AND, idx[18], idx[19]);
  idx[24] = subnetBuilder.addCell(AND, idx[19], Subnet::Link(idx[15].idx, true));

  // OR
  idx[25] = subnetBuilder.addCell(OR, idx[20], idx[21]);
  idx[26] = subnetBuilder.addCell(OR, idx[21], idx[22]);
  idx[27] = subnetBuilder.addCell(OR, idx[22], idx[23]);
  idx[28] = subnetBuilder.addCell(OR, idx[23], idx[24]);
  idx[29] = subnetBuilder.addCell(OR, idx[24], idx[20]);
  idx[30] = subnetBuilder.addCell(OR, idx[25], idx[26], idx[27], idx[28], idx[29]);

  // Out
  subnetBuilder.addCell(OUT, idx[30]);
  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, random) {
  const auto &subnet = Subnet::get(randomSubnet(10, 1, 150, 2, 3));
  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, minMor) {
  const auto &subnet = Subnet::get(randomSubnet(2, 1, 5, 2, 3));

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, fourZERO) {
  kitty::dynamic_truth_table tt(2);
  kitty::create_from_binary_string(tt, "0000");
  eda::gate::optimizer::synthesis::MMSynthesizer minato;
  const auto &subnet = minato.synthesize(tt).makeObject();

  DsdSynthesizer dsd;
  const auto &subnet2 = dsd.synthesize(tt).makeObject();
  LOG_DEBUG(subnet2);

//  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(subnet2));
}

TEST(DsdTest, wrong) {
  // ITE(a,or(b,e),(d,c)))
  /* Subnet creation */
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[40];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[2] = subnetBuilder.addCell(IN); //c
  idx[3] = subnetBuilder.addCell(IN); //d
  idx[4] = subnetBuilder.addCell(IN); //e
  idx[10] = subnetBuilder.addCell(OR, idx[1], idx[4]);
  idx[11] = subnetBuilder.addCell(OR, idx[3], idx[2]);
  idx[12] = addITE(idx[0], idx[10], idx[11], subnetBuilder);
  subnetBuilder.addCell(OUT, idx[12]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, orTest) {
  // OR(a,b)
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[6];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[4] = subnetBuilder.addCell(CellSymbol::OR, idx[0], idx[1]);
  subnetBuilder.addCell(OUT, idx[4]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, andTest) {
  // AND(a,b)
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[6];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[4] = subnetBuilder.addCell(AND, idx[0], idx[1]);
  subnetBuilder.addCell(OUT, idx[4]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, xorTest) {
  // xor(a,b)
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[6];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[4] = subnetBuilder.addCell(XOR, idx[0], idx[1]);
  subnetBuilder.addCell(OUT, idx[4]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, iteTest) {
  // xor(a,b)
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[6];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[2] = subnetBuilder.addCell(IN); //c
  idx[4] = addITE(idx[0], idx[1], idx[2], subnetBuilder);
  subnetBuilder.addCell(OUT, idx[4]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, notIteTest) {
  // xor(a,b)
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[6];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[2] = subnetBuilder.addCell(IN); //c
  idx[4] = addITE(idx[0], idx[1], idx[2], subnetBuilder, true);
  subnetBuilder.addCell(OUT, idx[4]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, iteWithCentralOr) {
  // ITE(c,(a + b),d)
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[6];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[2] = subnetBuilder.addCell(IN); //c
  idx[3] = subnetBuilder.addCell(IN); //d
  idx[4] = subnetBuilder.addCell(OR, idx[0], idx[1]);
  idx[5] = addITE(idx[2], idx[4], idx[3], subnetBuilder);
  subnetBuilder.addCell(OUT, idx[5]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, iteWithCentralAnd) {
  // ITE(c,(a * b),d)
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[6];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[2] = subnetBuilder.addCell(IN); //c
  idx[3] = subnetBuilder.addCell(IN); //d
  idx[4] = subnetBuilder.addCell(CellSymbol::AND, idx[0], idx[1]);
  idx[5] = addITE(idx[2], idx[4], idx[3], subnetBuilder);
  subnetBuilder.addCell(OUT, idx[5]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, iteWithFirstOr) {
  // ITE((a + b), c, d)
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[6];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[2] = subnetBuilder.addCell(IN); //c
  idx[3] = subnetBuilder.addCell(IN); //d
  idx[4] = subnetBuilder.addCell(CellSymbol::OR, idx[0], idx[1]);
  idx[5] = addITE(idx[4], idx[2], idx[3], subnetBuilder);
  subnetBuilder.addCell(OUT, idx[5]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, iteWithFirstAnd) {
  // ITE((a + b), c, d)
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[6];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[2] = subnetBuilder.addCell(IN); //c
  idx[3] = subnetBuilder.addCell(IN); //d
  idx[4] = subnetBuilder.addCell(CellSymbol::AND, idx[0], idx[1]);
  idx[5] = addITE(idx[4], idx[2], idx[3], subnetBuilder);
  subnetBuilder.addCell(OUT, idx[5]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, negationGettingFromOrActual) {
  // a * ((b XOR c) + d)
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[7];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[2] = subnetBuilder.addCell(IN); //c
  idx[3] = subnetBuilder.addCell(IN); //d
  idx[4] = subnetBuilder.addCell(XOR, idx[1], idx[2]);
  idx[5] = subnetBuilder.addCell(OR, idx[4], idx[3]);
  idx[6] = subnetBuilder.addCell(AND, idx[0], idx[5]);
  subnetBuilder.addCell(OUT, idx[6]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, notOrTest) {
  // Not(OR(a,b))
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[6];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[4] = subnetBuilder.addCell(CellSymbol::OR, idx[0], idx[1]);
  subnetBuilder.addCell(OUT, Subnet::Link(idx[4].idx, true));

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, AndNotTest) {
  // AND(a,!b)
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[6];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[4] = subnetBuilder.addCell(CellSymbol::AND, idx[0], Subnet::Link(idx[1].idx, true));
  subnetBuilder.addCell(OUT, idx[4]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, orOfNotTest) {
  // OR(Not(a),b)
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[6];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[4] = subnetBuilder.addCell(CellSymbol::OR, Subnet::Link(idx[0].idx, true), idx[1]);
  subnetBuilder.addCell(OUT, idx[4]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, orWithNotXor) {
  // (!a+b)^c
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[6];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[2] = subnetBuilder.addCell(IN); //c
  idx[4] = subnetBuilder.addCell(CellSymbol::OR, Subnet::Link(idx[0].idx, true), idx[1]);
  idx[5] = subnetBuilder.addCell(CellSymbol::XOR, idx[4], idx[2]);
  subnetBuilder.addCell(OUT, idx[5]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, iteFirstComplex) {
  // ITE([(!a+b)^c],c,d)]
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[7];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[2] = subnetBuilder.addCell(IN); //c
  idx[3] = subnetBuilder.addCell(IN); //d
  idx[4] = subnetBuilder.addCell(CellSymbol::OR, Subnet::Link(idx[0].idx, true), idx[1]);
  idx[5] = subnetBuilder.addCell(CellSymbol::XOR, idx[4], idx[2]);
  idx[6] = addITE(idx[5], idx[2], idx[3], subnetBuilder);
  subnetBuilder.addCell(OUT, idx[6]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, twoIteOneComplexOneNegation) {
  // !(ITE[e,f,ITE([(!a+b)^c],c,d)]+g)
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[13];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[2] = subnetBuilder.addCell(IN); //c
  idx[3] = subnetBuilder.addCell(IN); //d
  idx[4] = subnetBuilder.addCell(IN); //e
  idx[5] = subnetBuilder.addCell(IN); //f
  idx[6] = subnetBuilder.addCell(IN); //g
  idx[8] = subnetBuilder.addCell(CellSymbol::OR, Subnet::Link(idx[0].idx, true), idx[1]);
  idx[9] = subnetBuilder.addCell(CellSymbol::XOR, idx[8], idx[2]);
  idx[10] = addITE(idx[9], idx[2], idx[3], subnetBuilder);
  idx[11] = subnetBuilder.addCell(CellSymbol::OR, idx[10], idx[6]);
  idx[12] = addITE(idx[4], idx[5], idx[11], subnetBuilder);
  subnetBuilder.addCell(OUT, Subnet::Link(idx[12].idx, true));

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, aAndA) {
  //
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[13];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[8] = subnetBuilder.addCell(CellSymbol::OR, idx[0], idx[0]);
  subnetBuilder.addCell(OUT, idx[8]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, iteOrIte) {
  // !(ITE[ITE(a,b,c)+d,e,f])
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[13];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[2] = subnetBuilder.addCell(IN); //c
  idx[3] = subnetBuilder.addCell(IN); //d
  idx[4] = subnetBuilder.addCell(IN); //e
  idx[5] = subnetBuilder.addCell(IN); //f
  idx[10] = addITE(idx[0], idx[1], idx[2], subnetBuilder);
  idx[11] = subnetBuilder.addCell(CellSymbol::OR, idx[10], idx[3]);
  idx[12] = addITE(idx[11], idx[4], idx[5], subnetBuilder);
  subnetBuilder.addCell(OUT, Subnet::Link(idx[12].idx, true));

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, nonDisjointITE) {
  // ITE(a^d,b,c+d)
  /* Subnet creation */
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[40];
  idx[0] = subnetBuilder.addCell(IN); //a
  idx[1] = subnetBuilder.addCell(IN); //b
  idx[2] = subnetBuilder.addCell(IN); //c
  idx[5] = subnetBuilder.addCell(IN); //d
  idx[13] = subnetBuilder.addCell(XOR, idx[0], idx[5]);
  idx[14] = subnetBuilder.addCell(OR, idx[2], idx[5]);
  idx[15] = addITE(idx[13], idx[1], idx[14], subnetBuilder);
  subnetBuilder.addCell(OUT, idx[15]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, constantZero) {
  // 0
  /* Subnet creation */
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[2];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(ZERO, idx[0]);
  subnetBuilder.addCell(OUT, idx[1]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, constantZeroNegation) {
  // !0
  /* Subnet creation */
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[2];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(ZERO, idx[0]);
  subnetBuilder.addCell(OUT, Subnet::Link(idx[1].idx, true));

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, constantOne) {
  // 1
  /* Subnet creation */
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[2];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(ONE, idx[0]);
  subnetBuilder.addCell(OUT, idx[1]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, constantOneNegation) {
  // !1
  /* Subnet creation */
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[2];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(ONE, idx[0]);
  subnetBuilder.addCell(OUT, Subnet::Link(idx[1].idx, true));

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}

TEST(DsdTest, orWithZeroConstant) {
  // OR(a,0)
  /* Subnet creation */
  SubnetBuilder subnetBuilder;
  Subnet::Link idx[3];
  idx[0] = subnetBuilder.addCell(IN);
  idx[1] = subnetBuilder.addCell(ZERO);
  idx[2] = subnetBuilder.addCell(OR, idx[0], idx[1]);
  subnetBuilder.addCell(OUT, idx[2]);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  LOG_DEBUG(subnet);

  /* Simulation */
  EXPECT_TRUE(evaluate(subnet) == evaluate(handle(subnet)));
}
