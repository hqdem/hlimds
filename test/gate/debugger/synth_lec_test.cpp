//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021-2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/lec_test.h"
#include "gate/debugger/rnd_checker.h"
#include "gate/model/gnet_test.h"

/// LEC test suites are based on synthetic (manually constructed) nets.

//===----------------------------------------------------------------------===//
// SAT-based checker ('default')
//===----------------------------------------------------------------------===//
namespace eda::gate::debugger {

TEST(CheckGNetTest, CheckNorNorSmallTest) {
  EXPECT_TRUE(checkNorNorTest(8));
}

TEST(CheckGNetTest, CheckNorAndnSmallTest) {
  EXPECT_TRUE(checkNorAndnTest(8));
}

TEST(CheckGNetTest, CheckNorAndSmallTest) {
  EXPECT_FALSE(checkNorAndTest(8));
}

TEST(CheckGNetTest, CheckNorNorTest) {
  EXPECT_TRUE(checkNorNorTest(256));
}

TEST(CheckGNetTest, CheckNorAndnTest) {
  EXPECT_TRUE(checkNorAndnTest(256));
}

TEST(CheckGNetTest, CheckNorAndTest) {
  EXPECT_FALSE(checkNorAndTest(256));
}

//===----------------------------------------------------------------------===//
// Random simulation based checker ('rnd')
//===----------------------------------------------------------------------===//

TEST(RndChecker, SimpleTest) {

  Gate::SignalList inputs;
  Gate::Id output;

  auto net = makeNor(8, inputs, output);

  CheckerResult a = rndChecker(*net, 0, true);
  EXPECT_TRUE(a.notEqual());
}

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
  CheckerResult res = rndChecker(*mit, 0, true);
  CheckerResult res2 = rndChecker(*mit, 2, false);
  EXPECT_TRUE(res.equal());
  EXPECT_TRUE(res2.isUnknown());
  EXPECT_TRUE(mit->nSourceLinks() == netCloned->nSourceLinks());
}

//===----------------------------------------------------------------------===//
// Cross-checking tests
//===----------------------------------------------------------------------===//

TEST(CounterExample, OrAndTest) {
  auto net = GNet(0);
  SignalList inps;
  int countInp = 5;
  for (int i = 0; i < countInp; i++) {
    GateId z = net.addIn();
    inps.push_back(Signal::always(z));
  }
  GateId y = net.addGate(GateSymbol::OR, inps);
  net.addOut(y);

  std::unordered_map<Gate::Id, Gate::Id> testMap = {};
  auto netCloned = net.clone(testMap);
  net.setGate(y, GateSymbol::AND, inps);
  net.sortTopologically();
  netCloned->sortTopologically();

  RndChecker rnd;
  Checker def;

  CheckerResult resRnd = rnd.equivalent(net, *netCloned, testMap);
  CheckerResult resDef = def.equivalent(net, *netCloned, testMap);
  EXPECT_TRUE(resRnd.notEqual());
  EXPECT_TRUE(resDef.notEqual());

  Checker::Hints hints = makeHints(net, *netCloned, testMap);
  GNet *mit = miter(net, *netCloned, hints);
  Compiled compiled = makeCompiled(*mit);

  std::vector<bool> outRnd(1);
  std::vector<bool> outDef(1);
  compiled.simulate(outRnd, resRnd.getCounterExample());
  compiled.simulate(outDef, resDef.getCounterExample());
  EXPECT_TRUE(outDef == outRnd && outRnd[0] == 1);
}
} // namespace eda::gate::debugger
