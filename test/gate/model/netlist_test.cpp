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

// gate(x1, ..., xN).
static std::unique_ptr<Netlist> makeNet(GateSymbol gate,
                                        unsigned N,
                                        Signal::List &inputs,
                                        Gate::Id &outputId) {
  auto net = std::make_unique<Netlist>();

  for (unsigned i = 0; i < N; i++) {
    const Gate::Id inputId = net->addGate();
    const Signal input = Signal::always(inputId);
    inputs.push_back(input);
  }

  outputId = net->addGate(gate, inputs);
  return net;
}

// gate(~x1, ..., ~xN).
static std::unique_ptr<Netlist> makeNetn(GateSymbol gate,
                                         unsigned N,
                                         Signal::List &inputs,
                                         Gate::Id &outputId) {
  auto net = std::make_unique<Netlist>();

  Signal::List andInputs;
  for (unsigned i = 0; i < N; i++) {
    const Gate::Id inputId = net->addGate();
    const Signal input = Signal::always(inputId);
    inputs.push_back(input);

    const Gate::Id notGateId = net->addGate(GateSymbol::NOT, { input });
    const Signal andInput = Signal::always(notGateId);
    andInputs.push_back(andInput);
  }

  outputId = net->addGate(gate, andInputs);
  return net;
}

// (x1 | ... | xN).
std::unique_ptr<Netlist> makeOr(unsigned N,
                                Signal::List &inputs,
                                Gate::Id &outputId) {
  return makeNet(GateSymbol::OR, N, inputs, outputId);
}

// (x1 & ... & xN).
std::unique_ptr<Netlist> makeAnd(unsigned N,
                                 Signal::List &inputs,
                                 Gate::Id &outputId) {
  return makeNet(GateSymbol::AND, N, inputs, outputId);
}


// ~(x1 | ... | xN).
std::unique_ptr<Netlist> makeNor(unsigned N,
                                 Signal::List &inputs,
                                 Gate::Id &outputId) {
  return makeNet(GateSymbol::NOR, N, inputs, outputId);
}

// ~(x1 & ... & xN).
std::unique_ptr<Netlist> makeNand(unsigned N,
                                  Signal::List &inputs,
                                  Gate::Id &outputId) {
  return makeNet(GateSymbol::NAND, N, inputs, outputId);
}


// (~x1 | ... | ~xN).
std::unique_ptr<Netlist> makeOrn(unsigned N,
                                 Signal::List &inputs,
                                 Gate::Id &outputId) {
  return makeNetn(GateSymbol::OR, N, inputs, outputId);
}

// (~x1 & ... & ~xN).
std::unique_ptr<Netlist> makeAndn(unsigned N,
                                  Signal::List &inputs,
                                  Gate::Id &outputId) {
  return makeNetn(GateSymbol::AND, N, inputs, outputId);
}

TEST(NetlistTest, NetlistOrTest) {
  Signal::List inputs;
  Gate::Id outputId;
  auto net = makeOr(1024, inputs, outputId);
  EXPECT_TRUE(net != nullptr);
}

TEST(NetlistTest, NetlistAndTest) {
  Signal::List inputs;
  Gate::Id outputId;
  auto net = makeAnd(1024, inputs, outputId);
  EXPECT_TRUE(net != nullptr);
}

TEST(NetlistTest, NetlistNorTest) {
  Signal::List inputs;
  Gate::Id outputId;
  auto net = makeNor(1024, inputs, outputId);
  EXPECT_TRUE(net != nullptr);
}

TEST(NetlistTest, NetlistNandTest) {
  Signal::List inputs;
  Gate::Id outputId;
  auto net = makeNand(1024, inputs, outputId);
  EXPECT_TRUE(net != nullptr);
}

TEST(NetlistTest, NetlistOrnTest) {
  Signal::List inputs;
  Gate::Id outputId;
  auto net = makeOrn(1024, inputs, outputId);
  EXPECT_TRUE(net != nullptr);
}

TEST(NetlistTest, NetlistAndnTest) {
  Signal::List inputs;
  Gate::Id outputId;
  auto net = makeAndn(1024, inputs, outputId);
  EXPECT_TRUE(net != nullptr);
}
