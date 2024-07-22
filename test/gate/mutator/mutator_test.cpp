//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/bdd_checker.h"
#include "gate/debugger/fraig_checker.h"
#include "gate/debugger/rnd_checker.h"
#include "gate/debugger/sat_checker.h"
#include "gate/model/celltype.h"
#include "gate/model/examples.h"
#include "gate/model/subnet.h"
#include "gate/mutator/mutator.h"
#include "gate/translator/graphml_test_utils.h"
#include "util/env.h"

#include "gtest/gtest.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace eda::gate::mutator {

  using Cell = eda::gate::model::Subnet::Cell;
  using CellIdMap = std::unordered_map<size_t, size_t>;
  using CellType = eda::gate::model::CellType;
  using CellSymbol = eda::gate::model::CellSymbol;
  using eda::gate::debugger::BaseChecker;
  using eda::gate::debugger::options::BDD;
  using eda::gate::debugger::options::FRAIG;
  using eda::gate::debugger::options::RND;
  using eda::gate::debugger::options::SAT;
  using eda::gate::model::makeSubnet2AndOr;
  using eda::gate::model::makeSubnet2AndOr2;
  using Link = eda::gate::model::Subnet::Link;
  using LinkList = eda::gate::model::Subnet::LinkList;
  using Subnet = eda::gate::model::Subnet;
  using SubnetBuilder = eda::gate::model::SubnetBuilder;
  using SubnetID = eda::gate::model::SubnetID;

  /// Makes oldToNewMap for checkers
  CellIdMap makeMap(SubnetID &subnetID) {
    auto &net = Subnet::get(subnetID);
    CellIdMap mapOldToNew;
    for (size_t i = 0; i < net.getInNum(); i++) {
      mapOldToNew.insert(std::pair<CellID, CellID>(i, i));
    }
    for (size_t i = net.size() - net.getOutNum(); i < net.size(); i++) {
      mapOldToNew.insert(std::pair<CellID, CellID>(i, i));
    }
    return mapOldToNew;
  }

  //===------------------------------------------------------------------===//
  // Function which call checkers for mutated Subnet and default Subnet
  //===------------------------------------------------------------------===//
  template <typename CheckerType>
  bool usingCheckerForMutator(SubnetID subnetID, SubnetID &mutatedSubnetID, 
                              CheckerType &chk) {
    CellIdMap mapOldToNew = makeMap(subnetID);
    return chk.areEquivalent(subnetID, mutatedSubnetID, mapOldToNew).notEqual();
  }

  TEST(Mutator, andOr) {
    SubnetID subnetID = makeSubnet2AndOr(); 
    auto &net = Subnet::get(subnetID);
    CellIDList listCells = {0, 1, 2, 3, 4, 5, 6};
    CellSymbolList functions = {CellSymbol::AND};
    SubnetID mutatedSubnetID = Mutator::mutate(MutatorMode::CELL,
                                                net, 
                                                listCells, 
                                                functions);
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       BaseChecker::getChecker(BDD)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID,
                                       BaseChecker::getChecker(FRAIG)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       BaseChecker::getChecker(RND)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       BaseChecker::getChecker(SAT)));      
  }

  TEST(Mutator, andOr2) {
    SubnetID subnetID = makeSubnet2AndOr2(); 
    auto &net = Subnet::get(subnetID);
    CellIDList listCells = {0, 1, 2, 3, 4, 5, 6};
    CellSymbolList functions = {CellSymbol::AND, CellSymbol::OR};
    SubnetID mutatedSubnetID = Mutator::mutate(MutatorMode::CELL,
                                                net, 
                                                listCells, 
                                                functions);
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       BaseChecker::getChecker(BDD)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID,
                                       BaseChecker::getChecker(FRAIG)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       BaseChecker::getChecker(RND)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       BaseChecker::getChecker(SAT)));  
  }

  TEST(Mutator, numAndOr2) {
    SubnetID subnetID = makeSubnet2AndOr2(); 
    auto &net = Subnet::get(subnetID);
    int counter = 0;
    CellSymbolList functions = {CellSymbol::AND, CellSymbol::OR};
    SubnetID mutatedSubnetID = Mutator::mutate(MutatorMode::CELL,
                                               counter,
                                               net, 
                                               net.size(), 
                                               functions);
    EXPECT_EQ(counter, 1);
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       BaseChecker::getChecker(BDD)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID,
                                       BaseChecker::getChecker(FRAIG)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       BaseChecker::getChecker(RND)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       BaseChecker::getChecker(SAT))); 
  }

  TEST(Mutator, graphSs) {
    SubnetID subnetID = translator::translateGmlOpenabc("ss_pcm_orig")->make();
    auto &net = Subnet::get(subnetID);
    int counter = 0;
    CellSymbolList functions = {CellSymbol::AND, CellSymbol::OR};
    SubnetID mutatedSubnetID = Mutator::mutate(MutatorMode::CELL,
                                               counter,
                                               net, 
                                               60, 
                                               functions);
    EXPECT_EQ(counter, 60);
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       BaseChecker::getChecker(BDD)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID,
                                       BaseChecker::getChecker(FRAIG)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       BaseChecker::getChecker(SAT)));
  }

  TEST(Mutator, graphSasc) {
    SubnetID subnetID = translator::translateGmlOpenabc("sasc_orig")->make();
    auto &net = Subnet::get(subnetID);
    CellSymbolList functions = {CellSymbol::AND, CellSymbol::OR};
    SubnetID mutatedSubnetID = Mutator::mutate(MutatorMode::CELL,
                                                net, 
                                                5, 
                                                functions);
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       BaseChecker::getChecker(BDD)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID,
                                       BaseChecker::getChecker(FRAIG)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       BaseChecker::getChecker(SAT)));
  }
  
  TEST(Mutator, graphI2c) {
    SubnetID subnetID = translator::translateGmlOpenabc("i2c_orig")->make();
    auto &net = Subnet::get(subnetID);
    CellSymbolList functions = {CellSymbol::AND, CellSymbol::OR};
    SubnetID mutatedSubnetID = Mutator::mutate(MutatorMode::CELL,
                                                net, 
                                                15, 
                                                functions);
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       BaseChecker::getChecker(BDD)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       BaseChecker::getChecker(SAT)));
  }

  TEST(Mutator, cutAndOr2) {
    SubnetID subnetID = makeSubnet2AndOr(); 
    auto &net = Subnet::get(subnetID);
    CellIDList listCells = {6};
    CellSymbolList functions = {CellSymbol::AND, CellSymbol::OR};
    SubnetID mutatedSubnetID = Mutator::mutate(MutatorMode::CUT,
                                               net, 
                                               listCells, 
                                               functions,
                                               2);
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       BaseChecker::getChecker(BDD)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID,
                                       BaseChecker::getChecker(FRAIG)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       BaseChecker::getChecker(RND)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       BaseChecker::getChecker(SAT)));        
  }

  TEST(Mutator, cutNumAndOr2) {
    SubnetID subnetID = makeSubnet2AndOr(); 
    auto &net = Subnet::get(subnetID);
    CellSymbolList functions = {CellSymbol::AND, CellSymbol::OR};
    SubnetID mutatedSubnetID = Mutator::mutate(MutatorMode::CUT,
                                               net, 
                                               2, 
                                               functions,
                                               2);
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       BaseChecker::getChecker(BDD)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID,
                                       BaseChecker::getChecker(FRAIG)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       BaseChecker::getChecker(RND)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       BaseChecker::getChecker(SAT))); 
  }

  TEST(Mutator, cutGraphI2c) {
    SubnetID subnetID = translator::translateGmlOpenabc("i2c_orig")->make();
    auto &net = Subnet::get(subnetID);
    CellIDList list = {193, 195, 200};
    CellSymbolList functions = {CellSymbol::AND, CellSymbol::OR};
    SubnetID mutatedSubnetID = Mutator::mutate(MutatorMode::CUT,
                                                net, 
                                                list, 
                                                functions,
                                                3);
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       BaseChecker::getChecker(BDD)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       BaseChecker::getChecker(SAT)));
  }

  TEST(Mutator, cutGraphUsb) {
    SubnetID subnetID = translator::translateGmlOpenabc("usb_phy_orig")->make();
    auto &net = Subnet::get(subnetID);
    int counter = 0;
    CellSymbolList functions = {CellSymbol::AND};
    SubnetID mutatedSubnetID = Mutator::mutate(MutatorMode::CUT,
                                               counter,
                                               net, 
                                               2, 
                                               functions,
                                               2);
    EXPECT_EQ(counter, 4);
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       BaseChecker::getChecker(BDD)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID,
                                       BaseChecker::getChecker(FRAIG)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       BaseChecker::getChecker(SAT)));
  }
} // namespace eda::gate::mutator
