//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "../model/examples.h"
#include "../premapper/mapper/mapper_test.h"
#include "gate/debugger/k_checker.h"
#include "util/logging.h"

using KChecker = eda::gate::debugger::KChecker;
using Hints = eda::gate::debugger::SatChecker::Hints;
using CheckerResult = eda::gate::debugger::CheckerResult;

void createHints(GNet &net1, GNet &net2, Hints &hints) {
  GateBinding imap, omap;
  Gate::SignalList linputs, rinputs, loutputs, routputs;
  for (int i = net1.nGates() - 1; i >= 0; i--) {
    if (net1.gate(i)->isSource()) {
      linputs.push_back(Gate::Signal::always(net1.gate(i)->id()));
    }
    if (net1.gate(i)->isTarget()) {
      loutputs.push_back(Gate::Signal::always(net1.gate(i)->id()));
    }
  }
  for (int i = net2.nGates() - 1; i >= 0; i--) {
    if (net2.gate(i)->isSource()) {
      rinputs.push_back(Gate::Signal::always(net2.gate(i)->id()));
    }
    if (net2.gate(i)->isTarget()) {
      routputs.push_back(Gate::Signal::always(net2.gate(i)->id()));
    }
  }

  for (unsigned i = 0; i < net1.nSourceLinks(); i++) {
    imap.insert({Link(linputs[i].node()), Link(rinputs[i].node())});
  }
  for (unsigned i = 0; i < net1.nTargetLinks(); i++) {
    omap.insert({Link(loutputs[i].node()), Link(routputs[i].node())});
  }

  hints.sourceBinding = std::make_shared<GateBinding>(std::move(imap));
  hints.targetBinding = std::make_shared<GateBinding>(std::move(omap));
}

/* Structural hashing tests:
_______________________________

TwoInputAndTwoInputOrTest
net1: two-input And
net2: two-input Or
expected that hashTable contains: 2 primary outputs, 1st is inverted, 2nd isn't
expected amount of merged vertices: 0

TwoInputAndTwoInputAndTest
net1: two-input And
net2: two-input And
expected that hashTable contains: 1 not inverted primary output
expected amount of merged vertices: 1

ThreeInputAndThreeInputAndTest
net1: three-input And
net2: net with 3 primary inputs and 1 primary output
expected that hashTable contains: 1 vertex, 2 not inverted primary outputs
expected amount of merged vertices: 1

SixInputAndSixInputAndTest
net1: six-input And
net2: six-input And
expected that hashTable contains: 4 vertices, 1 not inverted primary output
expected amount of merged vertices: 5

SeveralPrimaryOutputsTest
net1: net with 4 primary inputs and 2 primary outputs
net2: net with 4 primary inputs and 2 primary outputs
expected that hashTable contains: 2 vertices, 4 not inverted primary outputs
expected amount of merged vertices: 0

SeveralGatesTest
net1 and net2 were taken from the article "Equivalence Checking Using Cuts And
Heaps" by A. Kuehlmann and F. Krohm (1997)
They both have 3 primary inputs and 1 primary output
expected that hashTable contains: 6 vertices, 2 inverted primary outputs
expected amount of merged vertices: 1
*/

TEST(KLecTest, TwoInputAndTwoInputOrTest) {
  std::shared_ptr<GNet> net1 = makeSingleGateNet(GateSymbol::AND, 2);
  std::shared_ptr<GNet> net2 = makeSingleGateNet(GateSymbol::OR, 2);
  net1->sortTopologically();
  net2->sortTopologically();

  KChecker checker;
  Hints hints;
  createHints(*net1, *net2, hints);
  EXPECT_TRUE(checker.equivalent(*net1, *net2, hints).isUnknown());

#ifdef UTOPIA_DEBUG
  checker.printHashTable();
#endif

  EXPECT_TRUE(checker.hashTableContains({0, 1, 0, 0}, 3));
  EXPECT_TRUE(checker.valueIsPrimaryOutput({0, 1, 0, 0}));
  EXPECT_FALSE(checker.primaryOutputSign({0, 1, 0, 0}));
  EXPECT_TRUE(checker.hashTableContains({0, 1, 1, 1}, 2));
  EXPECT_TRUE(checker.valueIsPrimaryOutput({0, 1, 1, 1}));
  EXPECT_TRUE(checker.primaryOutputSign({0, 1, 1, 1}));

  EXPECT_TRUE(checker.getMergedVertices().empty());
}

TEST(KLecTest, TwoInputAndTwoInputAndTest) {
  std::shared_ptr<GNet> net1 = makeSingleGateNet(GateSymbol::AND, 2);
  std::shared_ptr<GNet> net2 = makeSingleGateNet(GateSymbol::AND, 2);
  net1->sortTopologically();
  net2->sortTopologically();

  KChecker checker;
  Hints hints;
  createHints(*net1, *net2, hints);
  EXPECT_TRUE(checker.equivalent(*net1, *net2, hints).equal());

#ifdef UTOPIA_DEBUG
  checker.printHashTable();
#endif

  EXPECT_TRUE(checker.hashTableContains({0, 1, 1, 1}, 2));
  EXPECT_TRUE(checker.valueIsPrimaryOutput({0, 1, 1, 1}));
  EXPECT_TRUE(checker.primaryOutputSign({0, 1, 1, 1}));

  EXPECT_TRUE(checker.getMergedVertices().size() == 1);
  EXPECT_TRUE(checker.getMergedVertices()[0].first->id == 2);
  EXPECT_TRUE(checker.getMergedVertices()[0].second->id == 3);
}

TEST(KLecTest, ThreeInputAndThreeInputAndTest) {
  std::shared_ptr<GNet> net1 = makeSingleGateNet(GateSymbol::AND, 3);
  GNet net2;
  Gate::SignalList prInputs, inputs;

  for (int i = 0; i < 3; i++) {
    Gate::Id z = net2.addIn();
    prInputs.push_back(Gate::Signal::always(z));
  }
  inputs.push_back(prInputs[0]);
  inputs.push_back(prInputs[1]);
  Gate::Id x = net2.addAnd(inputs);
  inputs.clear();
  inputs.push_back(Gate::Signal::always(x));
  inputs.push_back(prInputs[2]);

  Gate::Id y = net2.addAnd(inputs);
  net2.addOut(y);

  net1->sortTopologically();
  net2.sortTopologically();

  KChecker checker;
  Hints hints;
  createHints(*net1, net2, hints);
  EXPECT_TRUE(checker.equivalent(*net1, net2, hints).equal());

#ifdef UTOPIA_DEBUG
  checker.printHashTable();
#endif

  EXPECT_TRUE(checker.hashTableContains({0, 1, 1, 1}, 3));
  EXPECT_TRUE(checker.hashTableContains({2, 3, 1, 1}, 4));
  EXPECT_TRUE(checker.valueIsPrimaryOutput({2, 3, 1, 1}));
  EXPECT_TRUE(checker.primaryOutputSign({2, 3, 1, 1}));
  EXPECT_TRUE(checker.hashTableContains({3, 2, 1, 1}, 6));
  EXPECT_TRUE(checker.valueIsPrimaryOutput({3, 2, 1, 1}));
  EXPECT_TRUE(checker.primaryOutputSign({3, 2, 1, 1}));

  EXPECT_TRUE(checker.getMergedVertices().size() == 2);
  EXPECT_TRUE(checker.getMergedVertices()[0].first->id == 3);
  EXPECT_TRUE(checker.getMergedVertices()[0].second->id == 5);
  EXPECT_TRUE(checker.getMergedVertices()[1].first->id == 4);
  EXPECT_TRUE(checker.getMergedVertices()[1].second->id == 6);
}

TEST(KLecTest, SixInputAndSixInputAndTest) {
  std::shared_ptr<GNet> net1 = makeSingleGateNet(GateSymbol::AND, 6);
  std::shared_ptr<GNet> net2 = makeSingleGateNet(GateSymbol::AND, 6);
  net1->sortTopologically();
  net2->sortTopologically();

  KChecker checker;
  Hints hints;
  createHints(*net1, *net2, hints);
  EXPECT_TRUE(checker.equivalent(*net1, *net2, hints).equal());

#ifdef UTOPIA_DEBUG
  checker.printHashTable();
#endif

  EXPECT_TRUE(checker.hashTableContains({0, 1, 1, 1}, 6));
  EXPECT_TRUE(checker.hashTableContains({2, 3, 1, 1}, 7));
  EXPECT_TRUE(checker.hashTableContains({4, 5, 1, 1}, 8));
  EXPECT_TRUE(checker.hashTableContains({6, 7, 1, 1}, 9));
  EXPECT_TRUE(checker.hashTableContains({8, 9, 1, 1}, 10));
  EXPECT_TRUE(checker.valueIsPrimaryOutput({8, 9, 1, 1}));
  EXPECT_TRUE(checker.primaryOutputSign({8, 9, 1, 1}));

  EXPECT_TRUE(checker.getMergedVertices().size() == 5);
  EXPECT_TRUE(checker.getMergedVertices()[0].first->id == 8);
  EXPECT_TRUE(checker.getMergedVertices()[0].second->id == 13);
  EXPECT_TRUE(checker.getMergedVertices()[1].first->id == 7);
  EXPECT_TRUE(checker.getMergedVertices()[1].second->id == 12);
  EXPECT_TRUE(checker.getMergedVertices()[2].first->id == 6);
  EXPECT_TRUE(checker.getMergedVertices()[2].second->id == 11);
  EXPECT_TRUE(checker.getMergedVertices()[3].first->id == 9);
  EXPECT_TRUE(checker.getMergedVertices()[3].second->id == 14);
  EXPECT_TRUE(checker.getMergedVertices()[4].first->id == 10);
  EXPECT_TRUE(checker.getMergedVertices()[4].second->id == 15);
}

TEST(KLecTest, SeveralPrimaryOutputsTest) {
  GNet net1, net2;
  eda::gate::optimizer::balanceSeveralOut(net1);

  Gate::SignalList inputs;
  for (int i = 0; i < 4; i++) {
    Gate::Id z = net2.addIn();
    inputs.push_back(Gate::Signal::always(z));
  }
  Gate::Id and1 = net2.addAnd(inputs[2], inputs[3]);
  Gate::Id and2 = net2.addAnd(inputs[1], Gate::Signal::always(and1));
  Gate::Id and3 = net2.addAnd(inputs[0], Gate::Signal::always(and2));
  net2.addOut(and3);
  net2.addOut(and2);

  net1.sortTopologically();
  net2.sortTopologically();

  KChecker checker;
  Hints hints;
  createHints(net1, net2, hints);
  EXPECT_TRUE(checker.equivalent(net1, net2, hints).isUnknown());

#ifdef UTOPIA_DEBUG
  checker.printHashTable();
#endif

  EXPECT_TRUE(checker.hashTableContains({0, 8, 1, 1}, 9));
  EXPECT_TRUE(checker.valueIsPrimaryOutput({0, 8, 1, 1}));
  EXPECT_TRUE(checker.primaryOutputSign({0, 8, 1, 1}));
  EXPECT_TRUE(checker.hashTableContains({2, 3, 1, 1}, 7));
  EXPECT_TRUE(checker.hashTableContains({0, 1, 1, 1}, 4));
  EXPECT_TRUE(checker.hashTableContains({1, 7, 1, 1}, 8));
  EXPECT_TRUE(checker.valueIsPrimaryOutput({1, 7, 1, 1}));
  EXPECT_TRUE(checker.primaryOutputSign({1, 7, 1, 1}));
  EXPECT_TRUE(checker.hashTableContains({4, 2, 1, 1}, 5));
  EXPECT_TRUE(checker.valueIsPrimaryOutput({4, 2, 1, 1}));
  EXPECT_TRUE(checker.primaryOutputSign({4, 2, 1, 1}));
  EXPECT_TRUE(checker.hashTableContains({5, 3, 1, 1}, 6));
  EXPECT_TRUE(checker.valueIsPrimaryOutput({5, 3, 1, 1}));
  EXPECT_TRUE(checker.primaryOutputSign({5, 3, 1, 1}));

  EXPECT_TRUE(checker.getMergedVertices().size() == 1);
  EXPECT_TRUE(checker.getMergedVertices()[0].first->id == 6);
  EXPECT_TRUE(checker.getMergedVertices()[0].second->id == 9);
}

TEST(KLecTest, SeveralGatesTest) {
  GNet net1, net2;
  Gate::SignalList prInputs;

  for (int i = 0; i < 3; i++) {
    Gate::Id z = net1.addIn();
    prInputs.push_back(Gate::Signal::always(z));
  }

  Gate::Id x = net1.addAnd({prInputs[0], prInputs[1]});
  prInputs.push_back(Gate::Signal::always(x));
  x = net1.addNand({prInputs[3], prInputs[2]});
  prInputs.push_back(Gate::Signal::always(x));
  x = net1.addNand({prInputs[3], prInputs[4]});
  prInputs.push_back(Gate::Signal::always(x));
  x = net1.addNand({prInputs[4], prInputs[2]});
  prInputs.push_back(Gate::Signal::always(x));
  x = net1.addNand({prInputs[5], prInputs[6]});
  prInputs.push_back(Gate::Signal::always(x));
  net1.addOut(x);

  prInputs.clear();
  for (int i = 0; i < 3; i++) {
    Gate::Id z = net2.addIn();
    prInputs.push_back(Gate::Signal::always(z));
  }

  x = net2.addNot(prInputs[0]);
  prInputs.push_back(Gate::Signal::always(x));
  x = net2.addNot(prInputs[1]);
  prInputs.push_back(Gate::Signal::always(x));
  x = net2.addOr({prInputs[3], prInputs[4]});
  prInputs.push_back(Gate::Signal::always(x));
  x = net2.addOr({prInputs[5], prInputs[2]});
  prInputs.push_back(Gate::Signal::always(x));
  x = net2.addNand({prInputs[5], prInputs[2]});
  prInputs.push_back(Gate::Signal::always(x));
  x = net2.addNand({prInputs[6], prInputs[7]});
  prInputs.push_back(Gate::Signal::always(x));
  net2.addOut(x);

  net1.sortTopologically();
  net2.sortTopologically();

  KChecker checker;
  Hints hints;
  createHints(net1, net2, hints);
  EXPECT_TRUE(checker.equivalent(net1, net2, hints).equal());

#ifdef UTOPIA_DEBUG
  checker.printHashTable();
#endif

  EXPECT_TRUE(checker.hashTableContains({3, 2, 1, 0}, 10));
  EXPECT_TRUE(checker.hashTableContains({3, 2, 0, 1}, 9));
  EXPECT_TRUE(checker.hashTableContains({0, 1, 1, 1}, 3));
  EXPECT_TRUE(checker.hashTableContains({3, 2, 1, 1}, 4));
  EXPECT_TRUE(checker.hashTableContains({4, 2, 0, 1}, 5));
  EXPECT_TRUE(checker.hashTableContains({3, 4, 1, 0}, 6));
  EXPECT_TRUE(checker.hashTableContains({6, 5, 0, 0}, 7));
  EXPECT_TRUE(checker.valueIsPrimaryOutput({6, 5, 0, 0}));
  EXPECT_FALSE(checker.primaryOutputSign({6, 5, 0, 0}));

  EXPECT_TRUE(checker.getMergedVertices().size() == 4);
  EXPECT_TRUE(checker.getMergedVertices()[0].first->id == 3);
  EXPECT_TRUE(checker.getMergedVertices()[0].second->id == 8);
  EXPECT_TRUE(checker.getMergedVertices()[1].first->id == 5);
  EXPECT_TRUE(checker.getMergedVertices()[1].second->id == 9);
  EXPECT_TRUE(checker.getMergedVertices()[2].first->id == 6);
  EXPECT_TRUE(checker.getMergedVertices()[2].second->id == 10);
  EXPECT_TRUE(checker.getMergedVertices()[3].first->id == 7);
  EXPECT_TRUE(checker.getMergedVertices()[3].second->id == 11);
}