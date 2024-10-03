//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//


#include "gate/simulator/simulator.h"
#include "gate/synthesizer/operation/addition.h"
#include "gate/synthesizer/operation/negation.h"

#include "gtest/gtest.h"

#include <algorithm>

using CellSymbol = eda::gate::model::CellSymbol;
using CellTypeAttr = eda::gate::model::CellTypeAttr;
using CellTypeAttrID = eda::gate::model::CellTypeAttrID;
using Simulator = eda::gate::simulator::Simulator;
using Subnet = eda::gate::model::Subnet;
using SubnetBuilder = eda::gate::model::SubnetBuilder;

std::pair<int32_t, int32_t> simulateAdder(uint16_t sizeA, uint16_t sizeB,
                                          uint16_t outSize, bool makeSub,
                                          bool generateAsSigned = false) {
  if (generateAsSigned) ++outSize;
  CellTypeAttr::PortWidths inputs = {sizeA, sizeB};
  CellTypeAttr::PortWidths outputs = {outSize};

  const CellTypeAttr &attr =
      CellTypeAttr::get(eda::gate::model::makeCellTypeAttr(inputs, outputs));

  SubnetBuilder result(
      makeSub ? (generateAsSigned ? eda::gate::synthesizer::synthSubS(attr)
                                  : eda::gate::synthesizer::synthSubU(attr))
              : (generateAsSigned ? eda::gate::synthesizer::synthAddS(attr)
                                  : eda::gate::synthesizer::synthAddU(attr)));

  Simulator simulator(result);
  Simulator::DataVector values(sizeA + sizeB);

  // when we generate digit as signed one, we need to save
  // one bit as zero, as it would be used as a sign bit
  uint16_t valA = std::rand() % (1 << (sizeA));
  int32_t val = valA;
  for (uint8_t di = 0; di < sizeA; ++di, val >>= 1) {
    values[di] = val & 1;
  }

  uint16_t valB = std::rand() % (1 << (sizeB));
  val = valB;
  for (uint8_t dj = 0; dj < sizeB; ++dj, val >>= 1) {
    values[sizeA + dj] = val & 1;
  }

  simulator.simulate(values);

  int32_t res = valA;
  val = valB;
  if (generateAsSigned && values[sizeA - 1]) {
    res |= ((1l << 32) - 1l) ^ ((1l << sizeA) - 1l);
  }
  if (generateAsSigned && values.back()) {
    val |= ((1l << 32) - 1l) ^ ((1l << sizeB) - 1l);
  }
  if (makeSub) {
    res -= val;
  } else {
    res += val;
  }

  int32_t resSimulated = 0u;
  for (int pos = outSize - 1; pos >= 0; --pos) {
    resSimulated <<= 1;
    resSimulated |= simulator.getOutput(pos) & 1;
  }

  uint32_t mask = (1 << outSize) - 1;
  res &= mask;
  resSimulated &= mask;
  if (res != resSimulated) {
    std::clog << valA << " " << valB << "; " << sizeA << " " << sizeB << '\n';
  }
  return {res & mask, resSimulated & mask};
}

TEST(Synthesizer, FullOutputLadnerFisherTestAdd) {
  const uint8_t start = 1u;
  const uint8_t end = 16u;

  srand(1u);

  for (uint8_t i = start; i <= end; ++i) {
    for (uint8_t j = start; j <= end; ++j) {
      auto result = simulateAdder(
          i, j, static_cast<uint16_t>(std::max(i, j) + 1u), false);

      EXPECT_EQ(result.first, result.second);
    }
  }
}

TEST(Synthesizer, PartOutputLadnerFisherTestAdd) {
  const uint8_t start = 1u;
  const uint8_t end = 16u;

  srand(1u);

  for (uint8_t i = start; i <= end; ++i) {
    for (uint8_t j = start; j <= end; ++j) {
      auto result = simulateAdder(i, j, (i + j) / 2, false);

      EXPECT_EQ(result.first, result.second);
    }
  }
}

TEST(Synthesizer, FullOutputLadnerFisherTestSub) {
  const uint8_t start = 1u;
  const uint8_t end = 15u;

  srand(1u);

  for (uint8_t i = start; i <= end; ++i) {
    for (uint8_t j = start; j <= end; ++j) {
      auto result = simulateAdder(i, j, std::max(i, j), true);

      EXPECT_EQ(result.first, result.second);
    }
  }
}

TEST(Synthesizer, PartOutputLadnerFisherTestSub) {
  const uint8_t start = 1u;
  const uint8_t end = 15u;

  srand(1u);

  for (uint8_t i = start; i <= end; ++i) {
    for (uint8_t j = start; j <= end; ++j) {
      auto result = simulateAdder(i, j, (i + j) / 2, true);

      EXPECT_EQ(result.first, result.second);
    }
  }
}

TEST(Synthesizer, WiderOutputLadnerFisherTestSub) {
  const uint8_t start = 2u;
  const uint8_t end = 8u;

  srand(1u);

  for (uint8_t i = start; i <= end; ++i) {
    for (uint8_t j = start; j <= end; ++j) {
      auto result = simulateAdder(i, j, 15u, true, true);

      EXPECT_EQ(result.first, result.second);

      if (result.first != result.second) {
        std::clog << i + 1 << " " << j + 1 << std::endl;
      }
    }
  }
}

TEST(Synthesizer, WiderOutputLadnerFisherTestSignedSum) {
  const uint8_t start = 2u;
  const uint8_t end = 8u;

  srand(1u);

  for (uint8_t i = start; i <= end; ++i) {
    for (uint8_t j = start; j <= end; ++j) {
      auto result = simulateAdder(i, j, 15u, false, true);

      EXPECT_EQ(result.first, result.second);

      if (result.first != result.second) {
        std::clog << i + 1 << " " << j + 1 << std::endl;
      }
    }
  }
}

TEST(Synthesizer, UnaryMinus) {
  const uint8_t start = 2u;
  const uint8_t end = 32u;

  srand(1u);

  for (uint8_t sizeA = start; sizeA <= end; ++sizeA) {

    CellTypeAttr::PortWidths inputs = {sizeA};
    CellTypeAttr::PortWidths outputs = {end};

    const CellTypeAttr &attr =
        CellTypeAttr::get(eda::gate::model::makeCellTypeAttr(inputs, outputs));

    SubnetBuilder result(eda::gate::synthesizer::synthNegU(attr)); // TODO: Add tests for NegS

    Simulator simulator(result);
    Simulator::DataVector values(sizeA);

    int32_t valA = std::rand() % (1 << (sizeA - 1));
    int32_t val = valA;
    for (uint8_t di = 0; di < sizeA; ++di, val >>= 1) {
      values[di] = val & 1;
    }

    simulator.simulate(values);

    int32_t resSimulated = 0u;

    for (int16_t pos = end - 1u; pos >= 0; --pos) { // FIXME: pos always >= 0
      resSimulated <<= 1;
      resSimulated |= simulator.getOutput(pos);
    }

    EXPECT_EQ(-valA, resSimulated);
  }
}
