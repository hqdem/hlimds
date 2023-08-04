//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/utils.h"
#include "gate/optimizer/examples.h"
#include "gate/optimizer/strategy/associative_balancer.h"
#include "gate/printer/dot.h"

#include "gtest/gtest.h"

namespace eda::gate::optimizer {

  std::string graphsPath = "test/data/gate/optimizer/output/associative_balance/";
  std::string balancedGraphsPath = graphsPath + "balanced/";
  std::string unbalancedGraphsPath = graphsPath + "unbalanced/";

  void testBalancer(const std::function<void(GNet &)> &netCreator,
                    const std::string &graphFileName,
                    const int expectedDepth) {
    
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    netCreator(net);

    Dot dot(&net);
    dot.print(unbalancedGraphsPath + graphFileName);
    
    AssociativeBalancer balancer(&net);
    balancer.balance();

    dot.print(balancedGraphsPath + graphFileName);

    EXPECT_EQ(expectedDepth, getNetDepth(net));
  }

  TEST(AssociativeBalanceTest, balanceAND) {
    testBalancer(balanceAND, "balanceAND.dot", 4);
  }

  TEST(AssociativeBalanceTest, balanceANDTwice) {
    testBalancer(balanceANDTwice, "balanceANDTwice.dot", 4);
  }

  TEST(AssociativeBalanceTest, balanceANDThrice) {
    testBalancer(balanceANDThrice, "balanceANDThrice.dot", 5);
  }

  TEST(AssociativeBalanceTest, unbalancableANDOR) {
    testBalancer(unbalancableANDOR, "unbalancableANDOR.dot", 7);
  }

  TEST(AssociativeBalanceTest, balanceOR) {
    testBalancer(balanceOR, "balanceOR.dot", 3);
  }

  TEST(AssociativeBalanceTest, balanceXORXNOR) {
    testBalancer(balanceXORXNOR, "balanceXORXNOR.dot", 3);
  }

  TEST(AssociativeBalanceTest, balanceSeveralOut) {
    testBalancer(balanceSeveralOut, "balanceSeveralOut.dot", 4);
  }

  TEST(AssociativeBalanceTest, gnet1) {
    testBalancer(gnet1, "gnet1.dot", 3);
  }

  TEST(AssociativeBalanceTest, oneInOneOut) {
    testBalancer(oneInOneOut, "oneInOneOut.dot", 1);
  }

} // namespace eda::gate::optimizer
