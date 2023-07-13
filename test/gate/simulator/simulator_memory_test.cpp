//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/simulator/simulator.h"

#include "gtest/gtest.h"

#include <iostream>
using namespace eda::gate::model;
using namespace eda::gate::simulator;

static Simulator simulator;

TEST(SimulatorMemoryTest, MemoryTest) {
  auto net = GNet(0);
  Gate::SignalList inps, xorInps, andInps;
  GNet::LinkList outs, ins;

  for (int i = 0; i < 2; i++) {
    Gate::Id z = net.addIn();
    inps.push_back(Gate::Signal::always(z));
  }

  Gate::Id gidOr = net.addGate(GateSymbol::OR, inps);
  andInps.push_back(Gate::Signal::always(gidOr));

  Gate::Id gidAnd = net.addGate(GateSymbol::AND, inps);
  xorInps.push_back(Gate::Signal::always(gidAnd));

  Gate::Id gidXor = net.addGate(GateSymbol::XOR, inps);
  xorInps.push_back(Gate::Signal::always(gidXor));

  Gate::Id gidXor2 = net.addGate(GateSymbol::XOR, xorInps);
  andInps.push_back(Gate::Signal::always(gidXor2));

  Gate::Id gidAnd2= net.addGate(GateSymbol::AND, andInps);

  Gate::Id gidNot = net.addGate(GateSymbol::NOT, Gate::Signal::always(gidAnd2));
  Gate::Id out = net.addOut(gidNot);
  outs.push_back(Gate::Link(out));

  for (auto input : inps) {
    ins.push_back(Gate::Link(input.node()));
  }

  net.sortTopologically();
  auto compiled = simulator.compile(net, ins, outs);
  std::uint64_t o;
  std::uint64_t i = 3;
  compiled.simulate(o, i);

  EXPECT_TRUE(compiled.getValue(gidOr) == 1);
  EXPECT_TRUE(compiled.getValue(gidAnd) == 1);
  EXPECT_TRUE(compiled.getValue(gidXor) == 0);
  EXPECT_TRUE(compiled.getValue(gidXor2) == 1);
  EXPECT_TRUE(compiled.getValue(gidAnd2) == 1);
  EXPECT_TRUE(compiled.getValue(gidNot) == 0);
  EXPECT_TRUE(o == 0);
}
