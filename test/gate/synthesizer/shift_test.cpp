//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/simulator/simulator.h"
#include "gate/synthesizer/operation/shift.h"

#include "gtest/gtest.h"

#include <algorithm>
#include <iostream>

using CellSymbol = eda::gate::model::CellSymbol;
using CellTypeAttr = eda::gate::model::CellTypeAttr;
using CellTypeAttrID = eda::gate::model::CellTypeAttrID;
using Simulator = eda::gate::simulator::Simulator;
using Subnet = eda::gate::model::Subnet;
using SubnetBuilder = eda::gate::model::SubnetBuilder;

std::pair<int32_t, int32_t> simulateShift(uint8_t inputSize, uint8_t shiftSize,
                                          uint8_t outSize, bool shiftL = true,
                                          bool useSign = false) {
  CellTypeAttr::PortWidths inputs = {inputSize, shiftSize};
  CellTypeAttr::PortWidths outputs = {outSize};

  const CellTypeAttr &attr =
      CellTypeAttr::get(eda::gate::model::makeCellTypeAttr(inputs, outputs));

  auto result = std::make_shared<SubnetBuilder>(
      shiftL ? (useSign ? eda::gate::synthesizer::synthShlS(attr)
                        : eda::gate::synthesizer::synthShlU(attr))
             : (useSign ? eda::gate::synthesizer::synthShrS(attr)
                        : eda::gate::synthesizer::synthShrU(attr)));
  Simulator simulator(result);
  Simulator::DataVector values(inputSize + shiftSize);

  uint16_t valA = std::rand() & ((1 << inputSize) - 1);
  uint16_t val = valA;
  for (uint8_t di = 0; di < inputSize; ++di, val >>= 1) {
    values[di] = val & 1;
  }

  uint16_t valB = std::rand() & ((1 << shiftSize) - 1);
  val = valB;
  for (uint8_t dj = 0; dj < shiftSize; ++dj, val >>= 1) {
    values[inputSize + dj] = val & 1;
  }

  simulator.simulate(values);

  int32_t res = (shiftL ? (valA << valB) : (valA >> valB)) &
                ((1 << attr.getOutWidth(0)) - 1);

  if (useSign && values[inputSize - 1u]) {
    int32_t fact = valA | (((1ull << 32) - 1) - ((1 << inputSize) - 1));
    res = (shiftL ? (fact << valB) : (fact >> valB)) &
                  ((1 << attr.getOutWidth(0)) - 1);
  }

  int32_t resSimulated = 0u;
  for (int pos = outSize - 1; pos >= 0; --pos) {
    resSimulated <<= 1;
    resSimulated |= simulator.getOutput(pos) & 1;
  }

  if (res != resSimulated) {
    std::clog << (int)inputSize << " " << (int)outSize << " " 
              << res << " " << valA << " " << valB
              << " " << resSimulated << std::endl;
    // std::clog << Subnet::get(result->make());
  }

  return {res, resSimulated};
}

TEST(Synthesizer, ShiftLs5s_3s) {
  auto [res, resSimulated] = simulateShift(5u, 3u, 13u, true, true);

  EXPECT_EQ(res, resSimulated);
}

TEST(Synthesizer, ShiftRu4u_4u) {
  auto [res, resSimulated] = simulateShift(4u, 4u, 4u, false, false);

  EXPECT_EQ(res, resSimulated);
}

TEST(Synthesizer, ShiftLuTestLargerOutput) {
  const uint8_t startInp = 4u, endInp = 16u;
  const uint8_t startSh = 2u, endSh = 6u;

  srand(1u);

  for (uint8_t inputSize = startInp; inputSize <= endInp; ++inputSize) {
    for (uint8_t shiftSize = startSh; shiftSize < endSh; ++shiftSize) {
      uint16_t outSize = static_cast<uint16_t>(inputSize + shiftSize);

      auto [res, resSimulated] = simulateShift(inputSize, shiftSize, outSize);

      EXPECT_EQ(res, resSimulated);
    }
  }
}


TEST(Synthesizer, ShiftLsTestLargerOutput) {
  const uint8_t startInp = 4u, endInp = 16u;
  const uint8_t startSh = 2u, endSh = 6u;

  srand(1u);

  for (uint8_t inputSize = startInp; inputSize <= endInp; ++inputSize) {
    for (uint8_t shiftSize = startSh; shiftSize < endSh; ++shiftSize) {
      uint16_t outSize = static_cast<uint16_t>(inputSize + shiftSize);

      auto [res, resSimulated] =
          simulateShift(inputSize, shiftSize, outSize, true, true);

      EXPECT_EQ(res, resSimulated);
    }
  }
}

TEST(Synthesizer, ShiftRuTestLargerOutput) {
  const uint8_t startInp = 4u, endInp = 16u;
  const uint8_t startSh = 2u, endSh = 6u;

  srand(1u);

  for (uint8_t inputSize = startInp; inputSize <= endInp; ++inputSize) {
    for (uint8_t shiftSize = startSh; shiftSize < endSh; ++shiftSize) {
      uint16_t outSize = static_cast<uint16_t>(inputSize + shiftSize);

      auto [res, resSimulated] =
          simulateShift(inputSize, shiftSize, outSize, false);

      EXPECT_EQ(res, resSimulated);
    }
  }
}

TEST(Synthesizer, ShiftRsTestLargerOutput) {
  const uint8_t startInp = 4u, endInp = 16u;
  const uint8_t startSh = 2u, endSh = 6u;

  srand(1u);

  for (uint8_t inputSize = startInp; inputSize <= endInp; ++inputSize) {
    for (uint8_t shiftSize = startSh; shiftSize < endSh; ++shiftSize) {
      uint16_t outSize = static_cast<uint16_t>(inputSize + shiftSize);

      auto [res, resSimulated] =
          simulateShift(inputSize, shiftSize, outSize, false, true);

      EXPECT_EQ(res, resSimulated);
    }
  }
}

TEST(Synthesizer, ShiftLTestRandOutput) {
  const uint8_t startInp = 4u, endInp = 16u;
  const uint8_t startSh = 2u, endSh = 6u;

  srand(1u);

  for (uint8_t inputSize = startInp; inputSize <= endInp; ++inputSize) {
    for (uint8_t shiftSize = startSh; shiftSize < endSh; ++shiftSize) {
      uint16_t outSize =
          static_cast<uint16_t>(rand() % (inputSize + shiftSize) + 1);

      auto [res, resSimulated] = simulateShift(inputSize, shiftSize, outSize);

      EXPECT_EQ(res, resSimulated);
    }
  }
}

TEST(Synthesizer, ShiftRuTestRandOutput) {
  const uint8_t startInp = 4u, endInp = 16u;
  const uint8_t startSh = 2u, endSh = 6u;

  srand(1u);

  for (uint8_t inputSize = startInp; inputSize <= endInp; ++inputSize) {
    for (uint8_t shiftSize = startSh; shiftSize < endSh; ++shiftSize) {
      uint16_t outSize =
          static_cast<uint16_t>(rand() % (inputSize + shiftSize) + 1);

      auto [res, resSimulated] =
          simulateShift(inputSize, shiftSize, outSize, false);

      EXPECT_EQ(res, resSimulated);
      if (res != resSimulated) {
        return;
      }
    }
  }
}

TEST(Synthesizer, ShiftRsTestRandOutput) {
  const uint8_t startInp = 4u, endInp = 16u;
  const uint8_t startSh = 2u, endSh = 6u;

  srand(1u);

  for (uint8_t inputSize = startInp; inputSize <= endInp; ++inputSize) {
    for (uint8_t shiftSize = startSh; shiftSize < endSh; ++shiftSize) {
      uint16_t outSize =
          static_cast<uint16_t>(rand() % (inputSize + shiftSize) + 1);

      auto [res, resSimulated] =
          simulateShift(inputSize, shiftSize, outSize, false, true);

      EXPECT_EQ(res, resSimulated);
      if (res != resSimulated) {
        return;
      }
    }
  }
}
