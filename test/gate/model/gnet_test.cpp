//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gnet_test.h"

#include "gtest/gtest.h"

#include <cassert>
#include <random>

using namespace eda::gate::model;

// gate(x1, ..., xN).
static std::unique_ptr<GNet> makeNet(GateSymbol gate,
                                     unsigned N,
                                     Signal::List &inputs,
                                     Gate::Id &outputId) {
  auto net = std::make_unique<GNet>();

  for (unsigned i = 0; i < N; i++) {
    const Gate::Id inputId = net->newGate();
    const Signal input = Signal::always(inputId);
    inputs.push_back(input);
  }

  outputId = net->addGate(gate, inputs);
  return net;
}

// gate(~x1, ..., ~xN).
static std::unique_ptr<GNet> makeNetn(GateSymbol gate,
                                      unsigned N,
                                      Signal::List &inputs,
                                      Gate::Id &outputId) {
  auto net = std::make_unique<GNet>();

  Signal::List andInputs;
  for (unsigned i = 0; i < N; i++) {
    const Gate::Id inputId = net->newGate();
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
std::unique_ptr<GNet> makeOr(unsigned N,
                             Signal::List &inputs,
                             Gate::Id &outputId) {
  return makeNet(GateSymbol::OR, N, inputs, outputId);
}

// (x1 & ... & xN).
std::unique_ptr<GNet> makeAnd(unsigned N,
                              Signal::List &inputs,
                              Gate::Id &outputId) {
  return makeNet(GateSymbol::AND, N, inputs, outputId);
}


// ~(x1 | ... | xN).
std::unique_ptr<GNet> makeNor(unsigned N,
                              Signal::List &inputs,
                              Gate::Id &outputId) {
  return makeNet(GateSymbol::NOR, N, inputs, outputId);
}

// ~(x1 & ... & xN).
std::unique_ptr<GNet> makeNand(unsigned N,
                               Signal::List &inputs,
                               Gate::Id &outputId) {
  return makeNet(GateSymbol::NAND, N, inputs, outputId);
}


// (~x1 | ... | ~xN).
std::unique_ptr<GNet> makeOrn(unsigned N,
                              Signal::List &inputs,
                              Gate::Id &outputId) {
  return makeNetn(GateSymbol::OR, N, inputs, outputId);
}

// (~x1 & ... & ~xN).
std::unique_ptr<GNet> makeAndn(unsigned N,
                               Signal::List &inputs,
                               Gate::Id &outputId) {
  return makeNetn(GateSymbol::AND, N, inputs, outputId);
}

// Random hierarchical network.
static std::unique_ptr<GNet> makeRand(std::size_t nGates,
                                      std::size_t nSubnets) {
  assert(nGates >= 2);
  auto net = std::make_unique<GNet>();

  // Create subnets.
  for (std::size_t i = 0; i < nSubnets; i++) {
    net->newSubnet();
  }

  // Create empty gates.
  const auto minGateId = net->newGate();
  for (std::size_t i = 0; i < nGates - 2; i++) {
    net->newGate();
  }
  const auto maxGateId = net->newGate();


  std::mt19937 gen(0);
  std::uniform_int_distribution<Gate::Id> snetDist(0, nSubnets - 1);
  std::uniform_int_distribution<Gate::Id> gateDist(minGateId, maxGateId);
  std::uniform_int_distribution<std::size_t> arityDist(0, 7);

  for (std::size_t n = 0; n < 4; n++) {
    // Create subnets.
    for (std::size_t i = 0; i < nSubnets; i++) {
      net->newSubnet();
    }

    // Randomly distributes the gates among the subnets.
    for (std::size_t i = 0; i < nGates; i++) {
      const auto gid = gateDist(gen);
      const auto dst = snetDist(gen);

      if (net->contains(gid)) {
        net->moveGate(gid, dst);
      }
    } // for: moveGate.

    // Randomly modify/connect the gates.
    for (std::size_t i = 0; i < nGates; i++) {
      const auto gid = gateDist(gen);

      if (net->contains(gid)) {
        Signal::List inputs;

        const std::size_t arity = arityDist(gen);
        for (std::size_t j = 0; j < arity; j++) {
          const auto inputId = gateDist(gen);
          const auto input = Signal::always(inputId);
          inputs.push_back(input);
        }

        net->setGate(gid, GateSymbol::AND, inputs);
      }
    } // for: setGate.

    // Randomly remove some gates.
    for (std::size_t i = 0; i < nGates / 16; i++) {
      const auto gid = gateDist(gen);

      if (net->contains(gid)) {
        net->removeGate(gid);
      }
    } // for: removeGate.

    net->groupOrphans();
    net->removeEmptySubnets();

    net->sortTopologically();

    net->flatten();
  } // top-level for.

  return net;
}

TEST(GNetTest, GNetOrTest) {
  Signal::List inputs;
  Gate::Id outputId;
  auto net = makeOr(1024, inputs, outputId);
  EXPECT_TRUE(net != nullptr);
}

TEST(GNetTest, GNetAndTest) {
  Signal::List inputs;
  Gate::Id outputId;
  auto net = makeAnd(1024, inputs, outputId);
  EXPECT_TRUE(net != nullptr);
}

TEST(GNetTest, GNetNorTest) {
  Signal::List inputs;
  Gate::Id outputId;
  auto net = makeNor(1024, inputs, outputId);
  EXPECT_TRUE(net != nullptr);
}

TEST(GNetTest, GNetNandTest) {
  Signal::List inputs;
  Gate::Id outputId;
  auto net = makeNand(1024, inputs, outputId);
  EXPECT_TRUE(net != nullptr);
}

TEST(GNetTest, GNetOrnTest) {
  Signal::List inputs;
  Gate::Id outputId;
  auto net = makeOrn(1024, inputs, outputId);
  EXPECT_TRUE(net != nullptr);
}

TEST(GNetTest, GNetAndnTest) {
  Signal::List inputs;
  Gate::Id outputId;
  auto net = makeAndn(1024, inputs, outputId);
  EXPECT_TRUE(net != nullptr);
}

TEST(GNetTest, GNetRandTest) {
  auto net = makeRand(1024, 256);
  EXPECT_TRUE(net != nullptr);
}
