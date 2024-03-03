//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/bdd_checker.h"
#include "gate/debugger/sat_checker.h"
#include "gate/debugger/rnd_checker.h"
#include "gate/model/examples.h"
#include "gate/parser/parser_test.h"
#include "gate/transformer/mutator/mutator.h"

#include <gtest/gtest.h>
#include <unordered_map>

namespace eda::gate::mutator {
  
  using CheckerResult = eda::gate::debugger::CheckerResult;
  using GateIdMap = std::unordered_map<GateId, GateId>;
  using eda::gate::debugger::getChecker;
  using eda::gate::debugger::options::BDD;
  using eda::gate::debugger::options::RND;
  using eda::gate::debugger::options::SAT;
  using eda::gate::optimizer::balanceAND;
  using eda::gate::optimizer::createLink;
  using eda::gate::optimizer::getNext;
  using eda::gate::parser::parseVerilog;
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
      mapOldToNew.insert(std::pair<GateId, GateId>(gate->id(), 
                        (gate->id() + gateNum)));
    }
    return mapOldToNew;
  }

  void printGraphs(GNet &gNet, std::string &fileName,
                   GNet &mutatedGNet, std::string &fileNameMutate, 
                   GateIdList &list) {
    std::string homePath = std::string(std::getenv("UTOPIA_HOME"));
    std::string fileDir = homePath + "/output/test/transformer/mutator/";
    std::string inputToPng = "cd " + fileDir + "\n" + "dot " + 
                fileName + ".dot" + " -Tpng -o " + fileName + ".png";
    std::string mutatedToPng = "cd " + fileDir + "\n" + "dot " + 
                fileNameMutate + ".dot" + " -Tpng -o " + fileNameMutate + ".png";
    std::system(("mkdir -p " + fileDir).c_str());
    Dot dot(&gNet);
    dot.print(fileDir + fileName + ".dot");
    std::system(inputToPng.c_str());
    Dot dotMutate(&mutatedGNet);
    dotMutate.fillColorGate(fileDir + fileNameMutate + ".dot", list);
    std::system(mutatedToPng.c_str());
  }
  
  //===------------------------------------------------------------------===//
  // Function which call checkers for mutated GNet and default GNet
  //===------------------------------------------------------------------===//
  template <typename templateChecker>
  bool usingCheckerForMutator(GNet &gNet, GNet &mutatedGNet, 
                              templateChecker &chk) {
    GateIdMap mapOldToNew = makeMap(gNet);
    CheckerResult chkResult = chk.equivalent(gNet, mutatedGNet, mapOldToNew);
    return chkResult.notEqual();
  }

//===---------------------------------------------------------------------===//
// Tests
//===---------------------------------------------------------------------===//

  TEST(Mutator, mutationBalanceAND) {
    GNet gNet;
    balanceAND(gNet);
    GNet mutatedGNet;
    GateIdList gates = {5, 6};
    mutatedGNet.addNet(Mutator::mutate(MutatorMode::GATE, gNet, gates));
    gNet.sortTopologically();
    mutatedGNet.sortTopologically();
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, getChecker(BDD)));
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, getChecker(RND)));
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, getChecker(SAT)));
    std::string fileName = "BalanceAND";
    std::string fileNameMutate = "BalanceANDMutate";
    printGraphs(gNet, fileName, mutatedGNet, fileNameMutate, gates);
  }

  TEST(Mutator, mutationAndOr) {
    GNet gNet;
    andOr(gNet);
    GNet mutatedGNet;
    int counter;
    mutatedGNet.addNet(Mutator::mutate(MutatorMode::GATE, counter, gNet, 3, 
                      {GateSymbol::AND, GateSymbol::OR}));
    gNet.sortTopologically();
    mutatedGNet.sortTopologically();
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, getChecker(BDD)));
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, getChecker(RND)));
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, getChecker(SAT)));
    EXPECT_EQ(counter, 3);
  }

  TEST(Mutator, gateHasNotOut) {
    GNet gNet;
    andOrWithoutOut(gNet);
    GNet mutatedGNet;
    int counter;
    mutatedGNet.addNet(Mutator::mutate(MutatorMode::GATE, counter, 
                                       gNet, gNet.nGates()));
    EXPECT_EQ(counter, 0);
  }

  TEST(Mutator, verilogAdder) {
    GNet gNet;
    gNet.addNet(*parseVerilog("adder.v"));
    GNet mutatedGNet;
    mutatedGNet.addNet(Mutator::mutate(MutatorMode::GATE, gNet, 3));
    gNet.sortTopologically();
    mutatedGNet.sortTopologically();
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, getChecker(SAT)));
    //TODO: BDD checker returns 'Killed'
  }

  TEST(Mutator, verilogC17) {
    GNet gNet;
    gNet.addNet(*parseVerilog("c17.v"));
    GNet mutatedGNet;
    GateIdList list;
    mutatedGNet.addNet(Mutator::mutate(MutatorMode::GATE, list, gNet, 
                                      gNet.nGates(), 
                                      {GateSymbol::NAND}));
    gNet.sortTopologically();
    mutatedGNet.sortTopologically();
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, getChecker(BDD)));
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, getChecker(RND)));
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, getChecker(SAT)));
    std::string fileName = "c17";
    std::string fileNameMutate = "c17Mutate";
    printGraphs(gNet, fileName, mutatedGNet, fileNameMutate, list);
  }

  TEST(Mutator, verilogSquare) {
    GNet gNet;
    gNet.addNet(*parseVerilog("square.v"));
    GNet mutatedGNet;
    mutatedGNet.addNet(Mutator::mutate(MutatorMode::GATE, gNet, 1));
    gNet.sortTopologically();
    mutatedGNet.sortTopologically();
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, getChecker(SAT)));
    //TODO: BDD checker returns 'Killed'
    //TODO: RND checker returns 'Failure' and 'Equal'
  }

  TEST(Mutator, cutBalanceAND) {
    GNet gNet;
    balanceAND(gNet);
    GNet mutatedGNet;
    GateIdList list;
    mutatedGNet.addNet(Mutator::mutate(MutatorMode::CUT, list, gNet, 
                                      1, {GateSymbol::AND}, 3));
    gNet.sortTopologically();
    mutatedGNet.sortTopologically();
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, getChecker(BDD)));
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, getChecker(RND)));
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, getChecker(SAT)));
    std::string fileName = "cutBalancedAnd";
    std::string fileNameMutate = "mutateCutBalanceAnd";
    printGraphs(gNet, fileName, mutatedGNet, fileNameMutate, list);
  }

  TEST(Mutator, cutAndOr) {
    GNet gNet;
    andOr(gNet);
    GNet mutatedGNet;
    GateIdList list = {6};
    mutatedGNet.addNet(Mutator::mutate(MutatorMode::CUT, gNet, list, 
                                      {GateSymbol::AND, GateSymbol::OR}, 3));
    gNet.sortTopologically();
    mutatedGNet.sortTopologically();
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, getChecker(BDD)));
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, getChecker(RND)));
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, getChecker(SAT)));
    std::string fileName = "cutAndOr";
    std::string fileNameMutate = "cutAndOrMutate";
    printGraphs(gNet, fileName, mutatedGNet, fileNameMutate, list);
  }

  TEST(Mutator, cutWithoutOut) {
    GNet gNet;
    andOrWithoutOut(gNet);
    GNet mutatedGNet;
    int counter;
    mutatedGNet.addNet(Mutator::mutate(MutatorMode::CUT, counter, gNet, 
                                       gNet.nGates()));
    EXPECT_EQ(counter, 0);
  }

  TEST(Mutator, cutNumberAndOr) {
    GNet gNet;
    andOr(gNet);
    GNet mutatedGNet;
    int counter;
    mutatedGNet.addNet(Mutator::mutate(MutatorMode::CUT, counter, gNet, 1, 
                                      {GateSymbol::AND, GateSymbol::OR}, 2));
    gNet.sortTopologically();
    mutatedGNet.sortTopologically();
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, getChecker(BDD)));
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, getChecker(RND)));
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, getChecker(SAT)));
    EXPECT_EQ(counter, 2);
  }

  TEST(Mutator, cutC17) {
    GNet gNet;
    gNet.addNet(*parseVerilog("c17.v"));
    GNet mutatedGNet;
    GateIdList list = {9};
    mutatedGNet.addNet(Mutator::mutate(MutatorMode::CUT, gNet, list, 
                                      {GateSymbol::NAND}, 3));
    gNet.sortTopologically();
    mutatedGNet.sortTopologically();
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, getChecker(BDD)));
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, getChecker(RND)));
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, getChecker(SAT)));
  }

  TEST(Mutator, cutNumberC499) {
    GNet gNet;
    gNet.addNet(*parseVerilog("c499.v"));
    GNet mutatedGNet;
    mutatedGNet.addNet(Mutator::mutate(MutatorMode::CUT, gNet, 3, 
                                      {GateSymbol::AND, GateSymbol::XOR}, 2));
    gNet.sortTopologically();
    mutatedGNet.sortTopologically();
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, getChecker(BDD)));
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, getChecker(RND)));
    EXPECT_TRUE(usingCheckerForMutator(gNet, mutatedGNet, getChecker(SAT)));
  }
} // namespace eda::gate::mutator

