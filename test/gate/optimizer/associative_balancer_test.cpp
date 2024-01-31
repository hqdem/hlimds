//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

//#define CHECKEQUAL

#include "gate/debugger/bdd_checker.h"
#include "gate/model/examples.h"
#include "gate/model/utils.h"
#include "gate/optimizer/optimizer_util.h"
#include "gate/optimizer/strategy/associative_balancer.h"
#include "gate/parser/parser_test.h"
#include "gate/premapper/mapper/mapper_test.h"
#include "gate/printer/dot.h"

#include "gtest/gtest.h"

namespace eda::gate::optimizer {
  using BddChecker = eda::gate::debugger::BddChecker;

  std::string graphsFolder = "associative_balance/";
  std::string unbalancedGraphsFolder = graphsFolder + "unbalanced/";
  std::string balancedGraphsFolder = graphsFolder + "balanced/";

  std::string testDataPath = "test/data/";
  std::string rilTestPath = testDataPath + "ril/ril_arithmetic_tests/";
  std::string verilogTestPath = testDataPath + "gate/optimizer/verilog/";

  bool areEquivalent(GNet *unbalancedNet , GNet *balancedNet, GateIdMap oldToNew) {
    return eda::gate::debugger::getChecker(
           eda::gate::debugger::options::BDD).equivalent(
           *balancedNet, *unbalancedNet, oldToNew).equal();
  }

  void printBalancingInfo(GNet *net,
                          const std::string &graphFileName,
                          const int expectedDepthBefore,
                          const int expectedDepthAfter) {

    EXPECT_EQ(expectedDepthBefore, getNetDepth(*net));

    std::cout << "Net depth before balancing: " << getNetDepth(*net) << std::endl;

    Dot dot(net);
    dot.print(createOutPath(unbalancedGraphsFolder).c_str() + graphFileName);

#ifdef CHECKEQUAL
    GateIdMap oldToNew;
    GNet *unbalancedNet = net->clone(oldToNew);
#endif

    AssociativeBalancer balancer(net);

    std::clock_t startBalance = clock();

    balancer.balance();

    clock_t endBalance = clock();

#ifdef CHECKEQUAL
    EXPECT_TRUE(areEquivalent(unbalancedNet, net, oldToNew));
#endif

    std::cout << "Net depth after balancing: " << getNetDepth(*net) << std::endl;
    std::cout << "Balances number: " << balancer.getBalancesNumber() << std::endl;
    double balancingTimeSec = (double)(endBalance - startBalance) / CLOCKS_PER_SEC;
    int balancingTimeMs = std::round(balancingTimeSec * 1000);
    std::cout << "Balancing time: " << balancingTimeMs << " ms" << std::endl;

    dot.print(createOutPath(balancedGraphsFolder).c_str() + graphFileName);

    EXPECT_EQ(expectedDepthAfter, getNetDepth(*net));
  }

  void testBalancer(const std::function<void(GNet &)> &netCreator,
                    const std::string &graphFileName,
                    const int expectedDepthBefore,
                    const int expectedDepthAfter) {

    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    netCreator(net);

    printBalancingInfo(&net, graphFileName,
                       expectedDepthBefore, expectedDepthAfter);
  }

  void testBalancerOnFile(const std::string &testFileName,
                          const std::string &graphFileName,
                          const bool needToPremap,
                          const int expectedDepthBefore,
                          const int expectedDepthAfter) {

    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet rawNet = getModel(testFileName,
                           rilTestPath,
                           eda::gate::parser::getExt(testFileName));
    rawNet.sortTopologically();
    std::shared_ptr<GNet> netToBalance = std::make_shared<GNet>(rawNet);
    if (needToPremap) {
      GateIdMap oldToNewGates;
      netToBalance = premap(std::make_shared<GNet>(rawNet),
                            oldToNewGates,
                            eda::gate::premapper::PreBasis::AIG
                            );
    }

    printBalancingInfo(netToBalance.get(), graphFileName,
                       expectedDepthBefore, expectedDepthAfter);
  }

  TEST(AssociativeBalanceTest, balanceAND) {
    testBalancer(balanceAND, "balanceAND.dot", 5, 4);
  }

  TEST(AssociativeBalanceTest, balanceAND2) {
    testBalancer(balanceAND2, "balanceAND2.dot", 4, 3);
  }

  TEST(AssociativeBalanceTest, balanceANDTwice) {
    testBalancer(balanceANDTwice, "balanceANDTwice.dot", 6, 4);
  }

  TEST(AssociativeBalanceTest, balanceANDThrice) {
    testBalancer(balanceANDThrice, "balanceANDThrice.dot", 7, 5);
  }

  TEST(AssociativeBalanceTest, unbalancableANDOR) {
    testBalancer(unbalancableANDOR, "unbalancableANDOR.dot", 7, 7);
  }

  TEST(AssociativeBalanceTest, balanceOR) {
    testBalancer(balanceOR, "balanceOR.dot", 4, 3);
  }

  TEST(AssociativeBalanceTest, balanceXORXNOR) {
    testBalancer(balanceXORXNOR, "balanceXORXNOR.dot", 4, 3);
  }

  TEST(AssociativeBalanceTest, balanceSeveralOut) {
    testBalancer(balanceSeveralOut, "balanceSeveralOut.dot", 4, 4);
  }

  TEST(AssociativeBalanceTest, balanceArity3) {
    testBalancer(balanceArity3, "balanceArity3.dot", 4, 3);
  }

  TEST(AssociativeBalanceTest, balanceArity4) {
    testBalancer(balanceArity4, "balanceArity4.dot", 4, 3);
  }

  TEST(AssociativeBalanceTest, balanceArity4_2) {
    testBalancer(balanceArity4_2, "balanceArity4_2.dot", 4, 3);
  }

  TEST(AssociativeBalanceTest, balanceArity4LR) {
    testBalancer(balanceArity4LR, "balanceArity4LR.dot", 4, 3);
  }

  TEST(AssociativeBalanceTest, gnet1) {
    testBalancer(gnet1, "gnet1.dot", 4, 3);
  }

  TEST(AssociativeBalanceTest, oneInOneOut) {
    testBalancer(oneInOneOut, "oneInOneOut.dot", 1, 1);
  }

  TEST(AssociativeBalanceTest, balanceMajLeft) {
    testBalancer(balanceMajLeft, "balanceMajLeft.dot", 4, 3);
  }

  TEST(AssociativeBalanceTest, balanceMajRight) {
    testBalancer(balanceMajRight, "balanceMajRight.dot", 4, 3);
  }

  TEST(AssociativeBalanceTest, balanceMajUnbalancable) {
    testBalancer(balanceMajUnbalancable, "balanceMajUnbalancable.dot", 4, 4);
  }

  TEST(AssociativeBalanceTest, balanceMaj2Variants) {
    testBalancer(balanceMaj2Variants, "balanceMaj2Variants.dot", 5, 4);
  }

  TEST(AssociativeBalanceTest, balanceMajTwice) {
    testBalancer(balanceMajTwice, "balanceMajTwice.dot", 5, 4);
  }

  TEST(AssociativeBalanceTest, addSmallRIL) {
    testBalancerOnFile("add_small.ril", "add_smallRIL.dot", false, 17, 17);
  }

  TEST(AssociativeBalanceTest, addRIL) {
    testBalancerOnFile("add.ril", "addRIL.dot", false, 33, 33);
  }

  TEST(AssociativeBalanceTest, mulSmallRIL) {
    testBalancerOnFile("mul_small.ril", "mul_smallRIL.dot", false, 22, 22);
  }

  TEST(AssociativeBalanceTest, mulRIL) {
    testBalancerOnFile("mul.ril", "mulRIL.dot", false, 62, 62);
  }

  TEST(AssociativeBalanceTest, subRIL) {
    testBalancerOnFile("sub.ril", "subRIL.dot", false, 34, 34);
  }

  TEST(AssociativeBalanceTest, testRIL) {
    testBalancerOnFile("test.ril", "testRIL.dot", true, 39, 38);
  }

  TEST(AssociativeBalanceTest, adderVerilog) {
    testBalancerOnFile("adder.v", "adderVerilog.dot", false, 511, 511);
  }

  TEST(AssociativeBalanceTest, c17Verilog) {
    testBalancerOnFile("c17.v", "c17Verilog.dot", false, 4, 4);
  }

  TEST(AssociativeBalanceTest, c17_modifiedVerilog) {
    testBalancerOnFile("c17_modified.v", "c17_modifiedVerilog.dot", false, 4, 4);
  }

  TEST(AssociativeBalanceTest, arbiterVerilog) {
    testBalancerOnFile("arbiter.v", "arbiterVerilog.dot", true, 175, 175);
  }

  TEST(AssociativeBalanceTest, barVerilog) {
    testBalancerOnFile("bar.v", "barVerilog.dot", true, 22, 22);
  }

  TEST(AssociativeBalanceTest, c1355Verilog) {
    testBalancerOnFile("c1355.v", "c1355Verilog.dot", true, 45, 45);
  }

  TEST(AssociativeBalanceTest, c1908Verilog) {
    testBalancerOnFile("c1908.v", "c1908Verilog.dot", true, 50, 50);
  }

  TEST(AssociativeBalanceTest, c3540Verilog) {
    testBalancerOnFile("c3540.v", "c3540Verilog.dot", true, 69, 63);
  }

  TEST(AssociativeBalanceTest, c432Verilog) {
    testBalancerOnFile("c432.v", "c432Verilog.dot", true, 40, 38);
  }

  TEST(AssociativeBalanceTest, c499Verilog) {
    testBalancerOnFile("c499.v", "c499Verilog.dot", true, 35, 35);
  }

  TEST(AssociativeBalanceTest, c6288Verilog) {
    testBalancerOnFile("c6288.v", "c6288Verilog.dot", false, 123, 123);
  }

  TEST(AssociativeBalanceTest, c880Verilog) {
    testBalancerOnFile("c880.v", "c880Verilog.dot", true, 41, 35);
  }

  TEST(AssociativeBalanceTest, cavlcVerilog) {
    testBalancerOnFile("cavlc.v", "cavlcVerilog.dot", true, 33, 33);
  }

  TEST(AssociativeBalanceTest, ctrlVerilog) {
    testBalancerOnFile("ctrl.v", "ctrlVerilog.dot", false, 20, 20);
  }

  TEST(AssociativeBalanceTest, decVerilog) {
    testBalancerOnFile("dec.v", "decVerilog.dot", false, 5, 5);
  }

  TEST(AssociativeBalanceTest, divVerilog) {
    testBalancerOnFile("div.v", "divVerilog.dot", true, 8737, 8709);
  }

  TEST(AssociativeBalanceTest, i2cVerilog) {
    testBalancerOnFile("i2c.v", "i2cVerilog.dot", true, 36, 34);
  }

  TEST(AssociativeBalanceTest, int2floatVerilog) {
    testBalancerOnFile("int2float.v", "int2floatVerilog.dot", true, 32, 31);
  }

  TEST(AssociativeBalanceTest, log2Verilog) {
    testBalancerOnFile("log2.v", "log2Verilog.dot", true, 767, 760);
  }

  TEST(AssociativeBalanceTest, maxVerilog) {
    testBalancerOnFile("max.v", "maxVerilog.dot", true, 508, 438);
  }

  TEST(AssociativeBalanceTest, multiplierVerilog) {
    testBalancerOnFile("multiplier.v", "multiplierVerilog.dot", true, 535, 530);
  }

  TEST(AssociativeBalanceTest, routerVerilog) {
    testBalancerOnFile("router.v", "routerVerilog.dot", true, 50, 46);
  }

  TEST(AssociativeBalanceTest, sinVerilog) {
    testBalancerOnFile("sin.v", "sinVerilog.dot", true, 337, 314);
  }

  TEST(AssociativeBalanceTest, sqrtVerilog) {
    testBalancerOnFile("sqrt.v", "sqrtVerilog.dot", true, 9330, 9196);
  }

  TEST(AssociativeBalanceTest, squareVerilog) {
    testBalancerOnFile("square.v", "squareVerilog.dot", true, 498, 497);
  }

  TEST(AssociativeBalanceTest, voterVerilog) {
    testBalancerOnFile("voter.v", "voterVerilog.dot", true, 137, 137);
  }

} // namespace eda::gate::optimizer
