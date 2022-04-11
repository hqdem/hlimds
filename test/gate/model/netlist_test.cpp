//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gtest/gtest.h"

#include "gate/model/netlist_test.h"

using namespace eda::gate::model;

// ~(x1 | ... | xN).
std::unique_ptr<Netlist> notOfOrs(unsigned N, Signal::List &inputs, unsigned &outputId) {
  auto net = std::make_unique<Netlist>();

  for (unsigned i = 0; i < N; i++) {
    const unsigned inputId = net->add_gate();
    const Signal input = net->always(inputId);
    inputs.push_back(input);
  }

  outputId = net->add_gate(GateSymbol::NOR, inputs);
  return net;
}

// (~x1 & ... & ~xN).
std::unique_ptr<Netlist> andOfNots(unsigned N, Signal::List &inputs, unsigned &outputId) {
  auto net = std::make_unique<Netlist>();

  Signal::List andInputs;
  for (unsigned i = 0; i < N; i++) {
    const unsigned inputId = net->add_gate();
    const Signal input = net->always(inputId);
    inputs.push_back(input);

    const unsigned notGateId = net->add_gate(GateSymbol::NOT, { input });
    const Signal andInput = net->always(notGateId);
    andInputs.push_back(andInput);
  }

  outputId = net->add_gate(GateSymbol::AND, andInputs);
  return net;
}

TEST(NetlistTest, NotOfOrsTest) {
  Signal::List inputs;
  unsigned outputId;
  auto net = notOfOrs(1024, inputs, outputId);
  EXPECT_TRUE(net != nullptr);
}

TEST(NetlistTest, AndOfNotsTest) {
  Signal::List inputs;
  unsigned outputId;
  auto net = andOfNots(1024, inputs, outputId);
  EXPECT_TRUE(net != nullptr);
}
