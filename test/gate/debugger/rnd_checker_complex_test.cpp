//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/miter.h"
#include "gate/debugger/rnd_checker.h"
#include "gate/model/gnet_test.h"

#include "gtest/gtest.h"

using namespace eda::gate::debugger;
using namespace eda::gate::model;

TEST(RndChecker, MiterAndCheckerTest) {
  auto net = GNet(0);
  SignalList inps;
  int countInp = 5;
  int countOut = 5;
  for (int i = 0; i < countInp; i++) {
    GateId z = net.addIn();
    inps.push_back(Signal::always(z));
  }
  GateId y = net.addGate(GateSymbol::OR, inps);
  net.addOut(y);
  SignalList inps1;
  for (int i = 0; i < countInp; i++) {
    GateId z = net.addIn();
    inps1.push_back(Signal::always(z));
  }
  GateId w = net.addGate(GateSymbol::OR, inps1);
  net.addOut(w);
  w = net.addGate(GateSymbol::OR, inps);
  for (int i = 0; i < countOut; i++) {
    net.addOut(w);
  }
  net.addOut(w);
  std::unordered_map<Gate::Id, Gate::Id> testMap = {};
  auto netCloned = net.clone(testMap);

  Checker::Hints hints = makeHints(net, *netCloned, testMap);
  GNet* mit = miter(net, *netCloned, hints);
  int res = rndChecker(*mit, 0, true);
  int res2 = rndChecker(*mit, 2, false);
  std::cout << "Result of RndChecker is:\t" << res << std::endl;
  EXPECT_TRUE(res == 0);
  EXPECT_TRUE(res2 == -1);
  EXPECT_TRUE(mit->nSourceLinks() == netCloned->nSourceLinks());
}
