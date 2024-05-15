//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#include "gate/debugger/bdd_checker.h"
#include "gate/debugger/fraig_checker.h"
#include "gate/debugger/sat_checker.h"
#include "gate/debugger/rnd_checker.h"
#include "gate/model2/celltype.h"
#include "gate/model2/examples.h"
#include "gate/model2/subnet.h"
#include "gate/parser/graphml_to_subnet.h"
#include "gate/transformer/mutator2/mutator2.h"

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
  using eda::gate::debugger::getChecker;
  using eda::gate::debugger::options::BDD;
  using eda::gate::debugger::options::FRAIG;
  using eda::gate::debugger::options::RND;
  using eda::gate::debugger::options::SAT;
  using eda::gate::model::make2AndOr;
  using eda::gate::model::make2AndOr2;
  using GraphMlSubnetParser = eda::gate::parser::graphml::GraphMlSubnetParser;
  using Link = eda::gate::model::Subnet::Link;
  using LinkList = eda::gate::model::Subnet::LinkList;
  using Subnet = eda::gate::model::Subnet;
  using SubnetBuilder = eda::gate::model::SubnetBuilder;
  using SubnetID = eda::gate::model::SubnetID;

  // Builds Subnet object from Verilog description
  SubnetID parseForTests(std::string fileName) {
    using path = std::filesystem::path;
    fileName += ".bench.graphml";
    const path home = std::string(getenv("UTOPIA_HOME"));
    const path dir = path("test") / "data" / "gate" / "parser"
    / "graphml" / "OpenABC" / "graphml_openabcd";
    std::string file = (home / dir / fileName).u8string();
    GraphMlSubnetParser parser;
    return parser.parse(file);
  }

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

  TEST(Mutator2, andOr) {
    SubnetID subnetID = make2AndOr(); 
    auto &net = Subnet::get(subnetID);
    CellIDList listCells = {0, 1, 2, 3, 4, 5, 6};
    CellSymbolList functions = {CellSymbol::AND};
    SubnetID mutatedSubnetID = Mutator2::mutate(Mutator2Mode::CELL,
                                                net, 
                                                listCells, 
                                                functions);
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       getChecker(BDD)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID,
                                       getChecker(FRAIG)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       getChecker(RND)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       getChecker(SAT)));      
  }

  TEST(Mutator2, andOr2) {
    SubnetID subnetID = make2AndOr2(); 
    auto &net = Subnet::get(subnetID);
    CellIDList listCells = {0, 1, 2, 3, 4, 5, 6};
    CellSymbolList functions = {CellSymbol::AND, CellSymbol::OR};
    SubnetID mutatedSubnetID = Mutator2::mutate(Mutator2Mode::CELL,
                                                net, 
                                                listCells, 
                                                functions);
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       getChecker(BDD)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID,
                                       getChecker(FRAIG)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       getChecker(RND)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       getChecker(SAT)));  
  }

  TEST(Mutator2, numAndOr2) {
    SubnetID subnetID = make2AndOr2(); 
    auto &net = Subnet::get(subnetID);
    int counter = 0;
    CellSymbolList functions = {CellSymbol::AND, CellSymbol::OR};
    SubnetID mutatedSubnetID = Mutator2::mutate(Mutator2Mode::CELL,
                                                counter,
                                                net, 
                                                net.size(), 
                                                functions);
    EXPECT_EQ(counter, 1);
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       getChecker(BDD)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID,
                                       getChecker(FRAIG)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       getChecker(RND)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       getChecker(SAT))); 
  }

  TEST(Mutator2, graphSs) {
    SubnetID subnetID = parseForTests("ss_pcm_orig");
    auto &net = Subnet::get(subnetID);
    int counter = 0;
    CellSymbolList functions = {CellSymbol::AND, CellSymbol::OR};
    SubnetID mutatedSubnetID = Mutator2::mutate(Mutator2Mode::CELL,
                                                counter,
                                                net, 
                                                60, 
                                                functions);
    EXPECT_EQ(counter, 60);
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       getChecker(BDD)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID,
                                       getChecker(FRAIG)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       getChecker(SAT)));
  }

  TEST(Mutator2, graphSasc) {
    SubnetID subnetID = parseForTests("sasc_orig");
    auto &net = Subnet::get(subnetID);
    CellSymbolList functions = {CellSymbol::AND, CellSymbol::OR};
    SubnetID mutatedSubnetID = Mutator2::mutate(Mutator2Mode::CELL,
                                                net, 
                                                5, 
                                                functions);
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       getChecker(BDD)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID,
                                       getChecker(FRAIG)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       getChecker(SAT)));
  }
  
  TEST(Mutator2, graphI2c) {
    SubnetID subnetID = parseForTests("i2c_orig");
    auto &net = Subnet::get(subnetID);
    CellSymbolList functions = {CellSymbol::AND, CellSymbol::OR};
    SubnetID mutatedSubnetID = Mutator2::mutate(Mutator2Mode::CELL,
                                                net, 
                                                15, 
                                                functions);
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       getChecker(BDD)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       getChecker(SAT)));
  }

  TEST(Mutator2, cutAndOr2) {
    SubnetID subnetID = make2AndOr(); 
    auto &net = Subnet::get(subnetID);
    CellIDList listCells = {6};
    CellSymbolList functions = {CellSymbol::AND, CellSymbol::OR};
    SubnetID mutatedSubnetID = Mutator2::mutate(Mutator2Mode::CUT,
                                                net, 
                                                listCells, 
                                                functions,
                                                2);
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       getChecker(BDD)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID,
                                       getChecker(FRAIG)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       getChecker(RND)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       getChecker(SAT)));        
  }

  TEST(Mutator2, cutNumAndOr2) {
    SubnetID subnetID = make2AndOr(); 
    auto &net = Subnet::get(subnetID);
    CellSymbolList functions = {CellSymbol::AND, CellSymbol::OR};
    SubnetID mutatedSubnetID = Mutator2::mutate(Mutator2Mode::CUT,
                                                net, 
                                                2, 
                                                functions,
                                                2);
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       getChecker(BDD)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID,
                                       getChecker(FRAIG)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       getChecker(RND)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       getChecker(SAT))); 
  }

  TEST(Mutator2, cutGraphI2c) {
    SubnetID subnetID = parseForTests("i2c_orig");
    auto &net = Subnet::get(subnetID);
    CellIDList list = {193, 195, 200};
    CellSymbolList functions = {CellSymbol::AND, CellSymbol::OR};
    SubnetID mutatedSubnetID = Mutator2::mutate(Mutator2Mode::CUT,
                                                net, 
                                                list, 
                                                functions,
                                                3);
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       getChecker(BDD)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       getChecker(SAT)));
  }

  TEST(Mutator2, cutGraphUsb) {
    SubnetID subnetID = parseForTests("usb_phy_orig");
    auto &net = Subnet::get(subnetID);
    int counter = 0;
    CellSymbolList functions = {CellSymbol::AND};
    SubnetID mutatedSubnetID = Mutator2::mutate(Mutator2Mode::CUT,
                                                counter,
                                                net, 
                                                2, 
                                                functions,
                                                2);
    EXPECT_EQ(counter, 4);
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       getChecker(BDD)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID,
                                       getChecker(FRAIG)));
    EXPECT_TRUE(usingCheckerForMutator(subnetID, mutatedSubnetID, 
                                       getChecker(SAT)));
  }
} // namespace eda::gate::mutator
