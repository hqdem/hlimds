//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/bdd_checker.h"
#include "gate/debugger/checker.h"
#include "gate/debugger/rnd_checker.h"
#include "gate/model/examples.h"
#include "gate/parser/parser_test.h"
#include "gate/transformer/mutator/mutator.h"

#include <gtest/gtest.h>
#include <unordered_map>

namespace eda::gate::mutator {
  
  using BddChecker = eda::gate::debugger::BddChecker;
  using Checker = eda::gate::debugger::Checker;
  using CheckerResult = eda::gate::debugger::CheckerResult;
  using GateIdMap = std::unordered_map<GateId, GateId>;
  using eda::gate::optimizer::balanceAND;
  using eda::gate::optimizer::createLink;
  using eda::gate::optimizer::getNext;
  using eda::gate::parser::parseVerilog;
  using RndChecker = eda::gate::debugger::RndChecker;
  using Signal = eda::gate::model::Gate::Signal;
  using SignalList = eda::gate::model::Gate::SignalList;
  using SubnetId = eda::gate::model::GNet::SubnetId;
  
  /* in1   in2                          */
  /* ┌─┐   ┌─┐                          */
  /* └─┘─┐ └─┘─┐                        */
  /* ┌─┐ |_┌─┐ |_┌─┐                    */
  /* └─┘───└─┘───└─┘─┐                  */
  /* in0  and4   and5|                  */
  /*             ┌─┐ |_┌─┐              */
  /*             └─┘───└─┘              */
  /*             in3   or6              */

  std::vector<GateId> andOr(GNet &gNet) {
    std::vector<GateId> g(4);
    for (GateId &elem : g) {
      elem = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}));
    g.push_back(createLink(gNet, g, {2, 4}));
    g.push_back(createLink(gNet, g, {3, 5}, model::GateSymbol::OR));
    g.push_back(createLink(gNet, g, {6}, model::GateSymbol::OUT));
    return g;
  }

  std::vector<GateId> andOrWithoutOut(GNet &gNet) {
    std::vector<GateId> g(4);
    for (GateId &elem : g) {
      elem = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}));
    g.push_back(createLink(gNet, g, {2, 4}));
    g.push_back(createLink(gNet, g, {3, 5}, model::GateSymbol::OR));
    return g;
  }
  
  /// Makes oldToNewMap for checkers
  GateIdMap makeMap(GNet &gNet) {
    GateIdMap mapOldToNew;
    size_t gateNum = gNet.nGates();
    for (auto *gate : gNet.gates()) {
      mapOldToNew.insert(std::pair<GateId, GateId>(gate->id(), (gate->id() + gateNum)));
    }
    return mapOldToNew;
  }
  
  //===----------------------------------------------------------------------===//
  // Function which call checkers for mutated GNet and default GNet
  //===----------------------------------------------------------------------===//
  template <typename templateChecker>
  bool usingCheckerForMutator(GNet &gNet, GNet &mutatedGNet, templateChecker &chk) {
    GateIdMap mapOldToNew = makeMap(gNet);
    CheckerResult chkResult = chk.equivalent(gNet, mutatedGNet, mapOldToNew);
    return chkResult.notEqual();
  }

//===----------------------------------------------------------------------===//
// Tests
//===----------------------------------------------------------------------===//

  TEST(Mutator, mutationBalanceAND) {
    GNet gNet;
    balanceAND(gNet);
    GNet mutatedGNet;
    GateIdList gates = {5, 6};
    mutatedGNet.addNet(Mutator::mutate(gNet, gates));
    gNet.sortTopologically();
    mutatedGNet.sortTopologically();
    Checker chk;
    BddChecker bddChk;
    RndChecker rndChk;
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, chk));
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, bddChk));
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, rndChk));
  }

  TEST(Mutator, mutationAndOr) {
    GNet gNet;
    andOr(gNet);
    GNet mutatedGNet;
    int counter;
    mutatedGNet.addNet(Mutator::mutate(counter, gNet, 3, {GateSymbol::AND, GateSymbol::OR}));
    gNet.sortTopologically();
    mutatedGNet.sortTopologically();
    Checker chk;
    BddChecker bddChk;
    RndChecker rndChk;
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, chk));
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, bddChk));
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, rndChk));
    EXPECT_EQ(counter, 3);
  }

  TEST(Mutator, gateHasNotOut) {
    GNet gNet;
    andOrWithoutOut(gNet);
    GNet mutatedGNet;
    int counter;
    mutatedGNet.addNet(Mutator::mutate(counter, gNet, gNet.nGates()));
    EXPECT_EQ(counter, 0);
  }

  TEST(Mutator, verilogAdder) {
    GNet gNet;
    gNet.addNet(*parseVerilog("adder.v"));
    GNet mutatedGNet;
    mutatedGNet.addNet(Mutator::mutate(gNet, 3));
    gNet.sortTopologically();
    mutatedGNet.sortTopologically();
    Checker chk;
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, chk));
    //TODO: BDD checker returns 'Killed'
  }

  TEST(Mutator, verilogArbiter) {
    GNet gNet;
    gNet.addNet(*parseVerilog("arbiter.v"));
    GNet mutatedGNet;
    mutatedGNet.addNet(Mutator::mutate(gNet, 3));
    gNet.sortTopologically();
    mutatedGNet.sortTopologically();
    Checker chk;
    BddChecker bddChk;
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, chk));
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, bddChk));
  }

  TEST(Mutator, verilogBar) {
    GNet gNet;
    gNet.addNet(*parseVerilog("bar.v"));
    GNet mutatedGNet;
    mutatedGNet.addNet(Mutator::mutate(gNet, gNet.nGates()));
    gNet.sortTopologically();
    mutatedGNet.sortTopologically();
    Checker chk;
    BddChecker bddChk;
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, chk));
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, bddChk));
  }

  TEST(Mutator, verilogC17) {
    GNet gNet;
    gNet.addNet(*parseVerilog("c17.v"));
    GNet mutatedGNet;
    mutatedGNet.addNet(Mutator::mutate(gNet, gNet.nGates(), {GateSymbol::NAND}));
    gNet.sortTopologically();
    mutatedGNet.sortTopologically();
    Checker chk;
    BddChecker bddChk;
    RndChecker rndChk;
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, chk));
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, bddChk));
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, rndChk));
  }

  TEST(Mutator, verilogC499) {
    GNet gNet;
    gNet.addNet(*parseVerilog("c499.v"));
    GNet mutatedGNet;
    mutatedGNet.addNet(Mutator::mutate(gNet, gNet.nGates()));
    gNet.sortTopologically();
    mutatedGNet.sortTopologically();
    Checker chk;
    BddChecker bddChk;
    RndChecker rndChk;
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, chk));
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, bddChk));
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, rndChk));
  }

  TEST(Mutator, verilogC1908) {
    GNet gNet;
    gNet.addNet(*parseVerilog("c1908.v"));
    GNet mutatedGNet;
    mutatedGNet.addNet(Mutator::mutate(gNet, gNet.nGates()));
    gNet.sortTopologically();
    mutatedGNet.sortTopologically();
    Checker chk;
    BddChecker bddChk;
    RndChecker rndChk;
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, chk));
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, bddChk));
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, rndChk));
  }

  TEST(Mutator, verilogSquare) {
    GNet gNet;
    gNet.addNet(*parseVerilog("square.v"));
    GNet mutatedGNet;
    mutatedGNet.addNet(Mutator::mutate(gNet, 1));
    gNet.sortTopologically();
    mutatedGNet.sortTopologically();
    Checker chk;
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, chk));
    //TODO: BDD checker returns 'Killed'
    //TODO: RND checker returns 'Failure' and 'Equal'
  }
} // namespace eda::gate::mutator
