//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/utils/bdd_dnf.h"
#include "gate/optimizer2/synthesis/dsd_to_subnet.h"

#include "gtest/gtest.h"

#include <iostream>

using namespace eda::gate::model::utils;
using namespace eda::gate::optimizer2::synthesis;

void checkDnfCorrectness(BDD &bdd, Cudd &cudd) {
  std::vector<kitty::cube> cubes = BddToDnf::getDnf(bdd);

  kitty::dynamic_truth_table tt(Cudd_ReadSize(bdd.manager()));
  kitty::create_from_cubes(tt, cubes);

  DsdSynthesizer dsd;

  auto first = dsd.synthesize(tt);
  auto second = dsd.synthesize({bdd.getNode(), cudd.getManager()});

  const auto &s1 = Subnet::get(first);
  const auto &s2 = Subnet::get(second);
  EXPECT_TRUE(evaluate(s1) == evaluate(s2));
}

TEST(BddToDnfTest, test1) {
  Cudd cudd;

  BDD x = cudd.bddVar();
  BDD z = cudd.bddVar();
  BDD y = cudd.bddVar();
  BDD h = cudd.bddVar();

  BDD bdd = h & !x & !y | !x & !z;

  checkDnfCorrectness(bdd, cudd);
}

TEST(BddToDnfTest, test2) {
  Cudd cudd;

  BDD x = cudd.bddVar();
  BDD y = cudd.bddVar();
  BDD z = cudd.bddVar();

  BDD bdd = x & y | !z;

  checkDnfCorrectness(bdd, cudd);
}

TEST(BddToDnfTest, test3) {
  Cudd cudd;

  BDD x = cudd.bddVar();
  BDD y = cudd.bddVar();

  BDD bdd = x & y;

  checkDnfCorrectness(bdd, cudd);
}

TEST(BddToDnfTest, test4) {
  Cudd cudd;

  BDD x = cudd.bddVar();
  BDD y = cudd.bddVar();

  BDD bdd = x | y;

  checkDnfCorrectness(bdd, cudd);
}

TEST(BddToDnfTest, test5) {
  Cudd cudd;

  BDD a = cudd.bddVar();
  BDD b = cudd.bddVar();
  BDD c = cudd.bddVar();
  BDD d = cudd.bddVar();
  BDD e = cudd.bddVar();
  BDD f = cudd.bddVar();

  BDD bdd = (a | b) & c | d ^ e ^ !f;

  checkDnfCorrectness(bdd, cudd);
}

TEST(BddToDnfTest, test6) {
  Cudd cudd;

  BDD a = cudd.bddVar();
  BDD bdd = !a;

  checkDnfCorrectness(bdd, cudd);
}
