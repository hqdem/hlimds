//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gnet_test.h"
#include "gate/transformer/bdd.h"

#include "gtest/gtest.h"

using namespace eda::gate::model;
using namespace eda::gate::transformer;

using BDDList = GNetBDDConverter::BDDList;
using GateBDDMap = GNetBDDConverter::GateBDDMap;
using GateList = GNetBDDConverter::GateList;
using GateUintMap = GNetBDDConverter::GateUintMap;

bool transformerAndTest() {
  Gate::SignalList inputs;
  Gate::Id outputId;
  std::shared_ptr<GNet> net = makeAnd(2, inputs, outputId);

  Cudd manager(0, 0);
  BDDList x = { manager.bddVar(), manager.bddVar() };
  GateBDDMap varMap;
  for (int i = 0; i < 2; i++) {
    varMap[inputs[i].node()] = x[i];
  }

  BDD netBDD = GNetBDDConverter::convert(*net, outputId, varMap, manager);
  BDD andBDD = x[0] & x[1];

  return netBDD == andBDD;
}

bool transformerOrTest() {
  Gate::SignalList inputs;
  Gate::Id outputId;
  std::shared_ptr<GNet> net = makeOr(2, inputs, outputId);

  Cudd manager(0, 0);
  BDDList x = { manager.bddVar(), manager.bddVar() };
  GateBDDMap varMap;
  for (int i = 0; i < 2; i++) {
    varMap[inputs[i].node()] = x[i];
  }

  BDD netBDD = GNetBDDConverter::convert(*net, outputId, varMap, manager);
  BDD orBDD = x[0] | x[1];

  return netBDD == orBDD;
}

bool transformerNorTest() {
  Gate::SignalList inputs;
  Gate::Id outputId1;
  std::shared_ptr<GNet> net = makeOr(2, inputs, outputId1);

  Cudd manager(0, 0);
  BDDList x = { manager.bddVar(), manager.bddVar() };
  GateBDDMap varMap;
  for (int i = 0; i < 2; i++) {
    varMap[inputs[i].node()] = x[i];
  }

  Gate::Id outputId2 = net->addGate(GateSymbol::NOT,
                                    {Gate::Signal::always(outputId1)});

  BDDList result;

  net->sortTopologically();
  GNetBDDConverter::convertList(*net, {outputId1, outputId2},
                                result, varMap, manager);
  BDD orBDD = x[0] | x[1];
  BDD norBDD = !(orBDD);

  return result[0] == orBDD && result[1] == norBDD;
}

TEST(TransformerBDDGNetTest, TransformerAndTest) {
  EXPECT_TRUE(transformerAndTest());
}

TEST(TransformerBDDGNetTest, TransformerOrTest) {
  EXPECT_TRUE(transformerOrTest());
}

TEST(TransformerBDDGNetTest, TransformerNorTest) {
  EXPECT_TRUE(transformerNorTest());
}
