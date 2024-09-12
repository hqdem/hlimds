//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/simulator/simulator.h"
#include "gate/synthesizer/operation/bitwise.h"

#include "gtest/gtest.h"

#include <algorithm>
#include <functional>

namespace eda::gate::synthesizer {

using CellSymbol = eda::gate::model::CellSymbol;
using CellTypeAttr = eda::gate::model::CellTypeAttr;
using CellTypeAttrID = eda::gate::model::CellTypeAttrID;
using Simulator = eda::gate::simulator::Simulator;
using SubnetID = eda::gate::model::SubnetID;
using Subnet = eda::gate::model::Subnet;
using SubnetBuilder = eda::gate::model::SubnetBuilder;
namespace synthesizer = eda::gate::synthesizer;
namespace model = eda::gate::model;

uint32_t simulateBitwise(uint16_t sizeA, uint16_t sizeB, uint16_t outSize,
    uint32_t valA, uint32_t valB,
    std::function<SubnetID(const CellTypeAttr &)> toSimulate) {

  CellTypeAttr::PortWidths inputs = {sizeA, sizeB};
  CellTypeAttr::PortWidths outputs = {outSize};

  const CellTypeAttr &attr =
      CellTypeAttr::get(model::makeCellTypeAttr(inputs, outputs));

  SubnetBuilder result(toSimulate(attr));
  Simulator simulator(result);
  Simulator::DataVector values(sizeA + sizeB);

  uint32_t val = valA;
  for (uint8_t di = 0; di < sizeA; ++di, val >>= 1) {
    values[di] = val & 1;
  }

  val = valB;
  for (uint8_t dj = 0; dj < sizeB; ++dj, val >>= 1) {
    values[sizeA + dj] = val & 1;
  }

  simulator.simulate(values);

  uint32_t resSimulated = 0u;

  for (int pos = outSize - 1; pos >= 0; --pos) {
    resSimulated <<= 1;
    resSimulated |= simulator.getOutput(pos);
  }

  return resSimulated;
}

TEST(Synthesizer, BitwiseAndUSizes_3_5_5_Values_5_13) {
  uint32_t res = simulateBitwise(3, 5, 5, 5, 13, synthesizer::synthBAndU);
  EXPECT_EQ(res, 5);
}

TEST(Synthesizer, BitwiseAndSSizes_3_5_5_Values_5_13) {
  uint32_t res = simulateBitwise(3, 5, 5, 5, 13, synthesizer::synthBAndS);
  EXPECT_EQ(res, 13);
}

TEST(Synthesizer, BitwiseNandUSizes_3_5_5_Values_5_13) {
  uint32_t res = simulateBitwise(3, 5, 5, 5, 13, synthesizer::synthBNandU);
  EXPECT_EQ(res, 26);
}

TEST(Synthesizer, BitwiseNandSSizes_3_5_5_Values_5_13) {
  uint32_t res = simulateBitwise(3, 5, 5, 5, 13, synthesizer::synthBNandS);
  EXPECT_EQ(res, 18);
}

TEST(Synthesizer, BitwiseOrUSizes_6_4_6_Values_3_6) {
  uint32_t res = simulateBitwise(6, 4, 6, 3, 6, synthesizer::synthBOrU);
  EXPECT_EQ(res, 7);
}

TEST(Synthesizer, BitwiseOrSSizes_7_3_7_Values_13_5) {
  uint32_t res = simulateBitwise(7, 3, 7, 13, 5, synthesizer::synthBOrS);
  EXPECT_EQ(res, 125);
}

TEST(Synthesizer, BitwiseNorUSizes_6_4_6_Values_3_6) {
  uint32_t res = simulateBitwise(6, 4, 6, 3, 6, synthesizer::synthBNorU);
  EXPECT_EQ(res, 56);
}

TEST(Synthesizer, BitwiseNorSSizes_7_3_7_Values_13_5) {
  uint32_t res = simulateBitwise(7, 3, 7, 13, 5, synthesizer::synthBNorS);
  EXPECT_EQ(res, 2);
}

TEST(Synthesizer, BitwiseXorUSizes_2_4_4_Values_2_13) {
  uint32_t res = simulateBitwise(2, 4, 4, 2, 13, synthesizer::synthBXorU);
  EXPECT_EQ(res, 15);
}

TEST(Synthesizer, BitwiseXorSSizes_2_4_4_Values_2_13) {
  uint32_t res = simulateBitwise(2, 4, 4, 2, 13, synthesizer::synthBXorS);
  EXPECT_EQ(res, 3);
}

TEST(Synthesizer, BitwiseXnorUSizes_6_9_12_Values_36_129) {
  uint32_t res = simulateBitwise(6, 9, 12, 36, 129, synthesizer::synthBXnorU);
  EXPECT_EQ(res, 3930);
}

TEST(Synthesizer, BitwiseXnorSSizes_6_9_3_Values_9_99) {
  uint32_t res = simulateBitwise(6, 9, 3, 9, 99, synthesizer::synthBXnorS);
  EXPECT_EQ(res, 5);
}
} // namespace eda::gate::synthesizer