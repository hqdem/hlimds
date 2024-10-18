//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/simulator/simulator.h"
#include "gate/synthesizer/operation/multiplication.h"

#include "gtest/gtest.h"

#include <algorithm>

using CellSymbol = eda::gate::model::CellSymbol;
using CellTypeAttr = eda::gate::model::CellTypeAttr;
using CellTypeAttrID = eda::gate::model::CellTypeAttrID;
using Simulator = eda::gate::simulator::Simulator;
using Subnet = eda::gate::model::Subnet;
using SubnetBuilder = eda::gate::model::SubnetBuilder;

std::pair<int32_t, int32_t>
simulateKaratsubaMultiplier(uint16_t sizeA, uint16_t sizeB, uint16_t outSize,
                            bool generateAsSigned = false) {
  CellTypeAttr::PortWidths inputs = {sizeA, sizeB};
  CellTypeAttr::PortWidths outputs = {outSize};

  const CellTypeAttr &attr =
      CellTypeAttr::get(eda::gate::model::makeCellTypeAttr(inputs, outputs));

  auto result = std::make_shared<SubnetBuilder>(
      generateAsSigned ? eda::gate::synthesizer::synthMulS(attr)
                       : eda::gate::synthesizer::synthMulU(attr));

  Simulator simulator(result);
  Simulator::DataVector values(sizeA + sizeB);

  // when we generate digit as signed one, we need to save
  // one bit as zero, as it would be used as a sign bit
  uint16_t valA = std::rand() & ((1 << (sizeA)) - 1u);
  int32_t val = valA;
  for (uint8_t di = 0; di < sizeA; ++di, val >>= 1) {
    values[di] = val & 1;
  }

  uint16_t valB = std::rand() & ((1 << (sizeB)) - 1u);
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

  res *= val;

  int32_t resSimulated = 0u;
  for (int pos = outSize - 1; pos >= 0; --pos) {
    resSimulated <<= 1;
    resSimulated |= simulator.getOutput(pos) & 1;
  }

  uint32_t mask = (1 << outSize) - 1;
  res &= mask;
  resSimulated &= mask;
  if (res != resSimulated) {
    // std::clog << Subnet::get(result->make());
    std::clog << valA << " " << valB << "; " << sizeA << " " << sizeB << '\n';
  }
  return {res, resSimulated};
}

TEST(Synthesizer, SimpleMultiplyerFullOutputUnsigned) {
  const uint8_t start = 1u;
  const uint8_t end = 4u;

  srand(1u);

  for (uint8_t i = start; i <= end; ++i) {
    for (uint8_t j = start; j <= end; ++j) {
      auto result = simulateKaratsubaMultiplier(
          i, j, static_cast<uint16_t>(std::max(i, j) << 1u));

      EXPECT_EQ(result.first, result.second);
    }
  }
}

TEST(Synthesizer, SimpleMultiplyerSmallOutputUnsigned) {
  const uint8_t start = 1u;
  const uint8_t end = 4u;

  srand(1u);

  for (uint8_t i = start; i <= end; ++i) {
    for (uint8_t j = start; j <= end; ++j) {
      auto result = simulateKaratsubaMultiplier(
          i, j, static_cast<uint16_t>(std::max(i, j)));

      EXPECT_EQ(result.first, result.second);
    }
  }
}

TEST(Synthesizer, SimpleMultiplyerFixedOutputUnsigned) {
  const uint8_t start = 1u;
  const uint8_t end = 4u;

  srand(1u);

  for (uint8_t i = start; i <= end; ++i) {
    for (uint8_t j = start; j <= end; ++j) {
      auto result = simulateKaratsubaMultiplier(i, j, 3u);

      EXPECT_EQ(result.first, result.second);
    }
  }
}

TEST(Synthesizer, SimpleMultiplyerFullOutputSigned) {
  const uint8_t start = 1u;
  const uint8_t end = 4u;

  srand(1u);

  for (uint8_t i = start; i <= end; ++i) {
    for (uint8_t j = start; j <= end; ++j) {
      auto result = simulateKaratsubaMultiplier(
          i, j, static_cast<uint16_t>(std::max(i, j) << 1u), true);

      EXPECT_EQ(result.first, result.second);
    }
  }
}

TEST(Synthesizer, SimpleMultiplyerSmallOutputSigned) {
  const uint8_t start = 1u;
  const uint8_t end = 4u;

  srand(1u);

  for (uint8_t i = start; i <= end; ++i) {
    for (uint8_t j = start; j <= end; ++j) {
      auto result = simulateKaratsubaMultiplier(
          i, j, static_cast<uint16_t>(std::max(i, j)), true);

      EXPECT_EQ(result.first, result.second);
    }
  }
}

TEST(Synthesizer, SimpleMultiplyerFixedOutputSigned) {
  const uint8_t start = 1u;
  const uint8_t end = 4u;

  srand(1u);

  for (uint8_t i = start; i <= end; ++i) {
    for (uint8_t j = start; j <= end; ++j) {
      auto result = simulateKaratsubaMultiplier(i, j, 16u, true);

      EXPECT_EQ(result.first, result.second);
    }
  }
}

TEST(Synthesizer, KaratsubaMultiplyerFullOutputUnsigned) {
  const uint8_t start = 5u;
  const uint8_t end = 8u;

  srand(1u);

  for (uint8_t i = start; i <= end; ++i) {
    for (uint8_t j = start; j <= end; ++j) {
      auto result = simulateKaratsubaMultiplier(
          i, j, static_cast<uint16_t>(std::max(i, j) << 1u));

      EXPECT_EQ(result.first, result.second);
      if (result.first != result.second) {
        return;
      }
    }
  }
}

TEST(Synthesizer, KaratsubaMultiplyerSmallOutputUnsigned) {
  const uint8_t start = 5u;
  const uint8_t end = 8u;

  srand(1u);

  for (uint8_t i = start; i <= end; ++i) {
    for (uint8_t j = start; j <= end; ++j) {
      auto result = simulateKaratsubaMultiplier(i, j, std::max(i, j));

      EXPECT_EQ(result.first, result.second);
      if (result.first != result.second) {
        return;
      }
    }
  }
}

TEST(Synthesizer, KaratsubaMultiplyerFixSmallOutputUnsigned) {
  const uint8_t start = 5u;
  const uint8_t end = 8u;

  srand(1u);

  for (uint8_t i = start; i <= end; ++i) {
    for (uint8_t j = start; j <= end; ++j) {
      auto result = simulateKaratsubaMultiplier(i, j, 6u);

      EXPECT_EQ(result.first, result.second);
      if (result.first != result.second) {
        return;
      }
    }
  }
}

TEST(Synthesizer, KaratsubaMultiplyerFixLargeOutputUnsigned) {
  const uint8_t start = 5u;
  const uint8_t end = 8u;

  srand(1u);

  for (uint8_t i = start; i <= end; ++i) {
    for (uint8_t j = start; j <= end; ++j) {
      auto result = simulateKaratsubaMultiplier(i, j, 20u);

      EXPECT_EQ(result.first, result.second);
      if (result.first != result.second) {
        return;
      }
    }
  }
}

TEST(Synthesizer, KaratsubaMultiplyerFullOutputSigned) {
  const uint8_t start = 5u;
  const uint8_t end = 8u;

  srand(1u);

  for (uint8_t i = start; i <= end; ++i) {
    for (uint8_t j = start; j <= end; ++j) {
      auto result = simulateKaratsubaMultiplier(
          i, j, static_cast<uint16_t>(std::max(i, j) << 1u), true);

      EXPECT_EQ(result.first, result.second);
      if (result.first != result.second) {
        return;
      }
    }
  }
}

TEST(Synthesizer, KaratsubaMultiplyerSmallOutputSigned) {
  const uint8_t start = 5u;
  const uint8_t end = 8u;

  srand(1u);

  for (uint8_t i = start; i <= end; ++i) {
    for (uint8_t j = start; j <= end; ++j) {
      auto result = simulateKaratsubaMultiplier(i, j, std::max(i, j), true);

      EXPECT_EQ(result.first, result.second);
      if (result.first != result.second) {
        return;
      }
    }
  }
}

TEST(Synthesizer, KaratsubaMultiplyerFixSmallOutputSigned) {
  const uint8_t start = 5u;
  const uint8_t end = 8u;

  srand(1u);

  for (uint8_t i = start; i <= end; ++i) {
    for (uint8_t j = start; j <= end; ++j) {
      auto result = simulateKaratsubaMultiplier(i, j, 6u, true);

      EXPECT_EQ(result.first, result.second);
      if (result.first != result.second) {
        return;
      }
    }
  }
}

TEST(Synthesizer, KaratsubaMultiplyerFixLargeOutputSigned) {
  const uint8_t start = 5u;
  const uint8_t end = 8u;

  srand(1u);

  for (uint8_t i = start; i <= end; ++i) {
    for (uint8_t j = start; j <= end; ++j) {
      auto result = simulateKaratsubaMultiplier(i, j, 20u, true);

      EXPECT_EQ(result.first, result.second);
      if (result.first != result.second) {
        return;
      }
    }
  }
}
