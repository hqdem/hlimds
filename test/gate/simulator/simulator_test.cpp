//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gnet_test.h"
#include "gate/simulator/simulator.h"

#include "gtest/gtest.h"

#include <iostream>

using namespace eda::gate::model;
using namespace eda::gate::simulator;

static bool simulatorTest(std::uint64_t N, const GNet &net, Gate::Id output) {
  Simulator simulator;
  Simulator::Compiled compiled = simulator.compile(net, {output});

  std::uint64_t o;
  for (std::uint64_t i = 0; i < N; i++) {
    compiled.simulate(o, i);
    std::cout << std::hex << i << " -> " << o << std::endl;
  }

  return true;
}

bool simulatorNorTest(unsigned N) {
  // ~(x1 | ... | xN).
  Signal::List inputs;
  Gate::Id output;

  auto net = makeNor(N, inputs, output);
  net->sortTopologically();

  return simulatorTest(1ull << N, *net, output);
}

bool simulatorAndnTest(unsigned N) {
  // (~x1 & ... & ~xN).
  Signal::List inputs;
  Gate::Id output;

  auto net = makeAndn(N, inputs, output);
  net->sortTopologically();

  return simulatorTest(1ull << N, *net, output);
}

TEST(SimulatorGNetTest, SimulatorNorTest) {
  EXPECT_TRUE(simulatorNorTest(4));
}

TEST(SimulatorGNetTest, SimulatorAndnTest) {
  EXPECT_TRUE(simulatorAndnTest(4));
}
