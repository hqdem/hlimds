//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/simulator/simulator.h"
#include "gate/synthesizer/operation/comparison.h"

#include "gtest/gtest.h"

#include <algorithm>
#include <functional>

using CellSymbol = eda::gate::model::CellSymbol;
using CellTypeAttr = eda::gate::model::CellTypeAttr;
using CellTypeAttrID = eda::gate::model::CellTypeAttrID;
using Simulator = eda::gate::simulator::Simulator;
using SubnetID = eda::gate::model::SubnetID;
using Subnet = eda::gate::model::Subnet;
using SubnetBuilder = eda::gate::model::SubnetBuilder;

bool simulateComparator(
    uint16_t sizeA, uint16_t sizeB,
    std::function<bool(int16_t, int16_t)> operation,
    std::function<SubnetID(const CellTypeAttr &)> toSimulate,
    const bool useSign, const bool makeEqual) {

  CellTypeAttr::PortWidths inputs = {sizeA, sizeB};
  CellTypeAttr::PortWidths outputs = {1u};

  const CellTypeAttr &attr =
      CellTypeAttr::get(eda::gate::model::makeCellTypeAttr(inputs, outputs));

  auto result = std::make_shared<SubnetBuilder>(toSimulate(attr));
  Simulator simulator(result);
  Simulator::DataVector values(sizeA + sizeB);

  int32_t valA = std::rand() % (1 << sizeA);
  int32_t val = valA;
  for (uint8_t di = 0; di < sizeA; ++di, val >>= 1) {
    values[di] = val & 1;
  }

  int32_t valB = makeEqual ? valA : std::rand() % (1 << sizeB);
  val = valB;
  for (uint8_t dj = 0; dj < sizeB; ++dj, val >>= 1) {
    values[sizeA + dj] = val & 1;
  }

  simulator.simulate(values);
  if (useSign) {
    if (values.back()) {
      valB |= (1 << 16) - 1 - ((1 << sizeB) - 1);
    }

    if (values[sizeA - 1u]) {
      valA |= (1 << 16) - 1 - ((1 << sizeA) - 1);
    }
  }

  bool targetResult = operation(valA, valB);
  targetResult = targetResult == (simulator.getOutput(0) & 1);

  if (!targetResult) {
    std::clog << operation(valA, valB) << " " << valA << " " << valB << " "
              << (simulator.getOutput(0) & 1) << std::endl;
    std::clog << Subnet::get(result->make());
  }

  if (sizeA == 8u && sizeB == 4u) {
    std::clog << Subnet::get(result->make());
  }

  return targetResult;
}

void subTest(std::function<bool(int16_t, int16_t)> operation,
             std::function<SubnetID(const CellTypeAttr &)> toSimulate,
             bool useSign = false, bool makeEqual = false) {
  const uint8_t start = 1u;
  const uint8_t end = 15u;

  srand(1u);

  for (uint8_t i = start; i <= end; ++i) {
    for (uint8_t j = start; j <= end; ++j) {
      bool result = simulateComparator(i, makeEqual ? i : j, operation,
                                       toSimulate, useSign, makeEqual);

      EXPECT_TRUE(result);
    }
  }
}

// Tests for equality
TEST(Synthesizer, EqualityUnsigned) {
  const auto operation = [](int16_t a, int16_t b) { return a == b; };
  subTest(operation, eda::gate::synthesizer::synthEqU);
}

TEST(Synthesizer, EqualityUnsignedAlwaysEqual) {
  const auto operation = [](int16_t a, int16_t b) { return a == b; };
  subTest(operation, eda::gate::synthesizer::synthEqU, false, true);
}

TEST(Synthesizer, EqualitySigned) {
  const auto operation = [](int16_t a, int16_t b) { return a == b; };
  subTest(operation, eda::gate::synthesizer::synthEqS, true);
}

TEST(Synthesizer, EqualitySignedAlwaysEqual) {
  const auto operation = [](int16_t a, int16_t b) { return a == b; };
  subTest(operation, eda::gate::synthesizer::synthEqS, true, true);
}
// Equality tests end

// Tests for unequality
TEST(Synthesizer, UnequalityUnsigned) {
  const auto operation = [](int16_t a, int16_t b) { return a != b; };
  subTest(operation, eda::gate::synthesizer::synthNeqU);
}

TEST(Synthesizer, UnequalityUnsignedAlwaysEqual) {
  const auto operation = [](int16_t a, int16_t b) { return a != b; };
  subTest(operation, eda::gate::synthesizer::synthNeqU, false, true);
}

TEST(Synthesizer, UnequalitySigned) {
  const auto operation = [](int16_t a, int16_t b) { return a != b; };
  subTest(operation, eda::gate::synthesizer::synthNeqS, true);
}

TEST(Synthesizer, UnequalitySignedAlwaysEqual) {
  const auto operation = [](int16_t a, int16_t b) { return a != b; };
  subTest(operation, eda::gate::synthesizer::synthNeqS, true, true);
}
// Tests for unequality end

// Tests for greater than
TEST(Synthesizer, GreaterThanUnsigned) {
  const auto operation = [](int16_t a, int16_t b) { return a > b; };
  subTest(operation, eda::gate::synthesizer::synthGtU);
}

TEST(Synthesizer, GreaterThanSigned) {
  const auto operation = [](int16_t a, int16_t b) { return a > b; };
  subTest(operation, eda::gate::synthesizer::synthGtS, true);
}
// grater than tests end

// Tests for greater than or equal
TEST(Synthesizer, GreaterThanOrEqualUnsigned) {
  const auto operation = [](int16_t a, int16_t b) { return a >= b; };
  subTest(operation, eda::gate::synthesizer::synthGteU);
}

TEST(Synthesizer, GreaterThanOrEqualSigned) {
  const auto operation = [](int16_t a, int16_t b) { return a >= b; };
  subTest(operation, eda::gate::synthesizer::synthGteS, true);
}
// grater than tests end or equal

// Tests for greater than
TEST(Synthesizer, LessThanUnsigned) {
  const auto operation = [](int16_t a, int16_t b) { return a < b; };
  subTest(operation, eda::gate::synthesizer::synthLtU);
}

TEST(Synthesizer, LessThanSigned) {
  const auto operation = [](int16_t a, int16_t b) { return a < b; };
  subTest(operation, eda::gate::synthesizer::synthLtS, true);
}
// grater than tests end

// Tests for greater than or equal
TEST(Synthesizer, LessThanOrEqualUnsigned) {
  const auto operation = [](int16_t a, int16_t b) { return a <= b; };
  subTest(operation, eda::gate::synthesizer::synthLteU);
}

TEST(Synthesizer, LessThanOrEqualSigned) {
  const auto operation = [](int16_t a, int16_t b) { return a <= b; };
  subTest(operation, eda::gate::synthesizer::synthLteS, true);
}
// grater than tests end or equal
