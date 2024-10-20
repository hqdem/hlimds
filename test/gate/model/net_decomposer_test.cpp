//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021-2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/decomposer/net_decomposer.h"
#include "gate/model/generator/layer_generator.h"
#include "gate/model/printer/net_printer.h"

#include "gtest/gtest.h"

#include <cstdint>
#include <iostream>
#include <vector>

namespace eda::gate::model {

static NetDecomposer::Result decomposeNet(NetID netID) {
#ifdef UTOPIA_DEBUG
  std::cout << Net::get(netID) << std::endl;
#endif // UTOPIA_DEBUG

  NetDecomposer::Result result;
  NetDecomposer::get().decompose(netID, result);

#ifdef UTOPIA_DEBUG
  for (const auto &subnetInfo : result.subnets) {
    std::cout << Subnet::get(subnetInfo.subnetID) << std::endl;
  }
#endif // UTOPIA_DEBUG

#ifdef UTOPIA_DEBUG
  const auto newNetID = NetDecomposer::get().compose(result);
  std::cout << Net::get(newNetID) << std::endl;
#endif // UTOPIA_DEBUG

  return result;
}

TEST(NetDecomposerTest, SimplePosTest) {
  NetBuilder netBuilder;

  const auto input1 = makeCell(IN);
  netBuilder.addCell(input1);
  const auto input2 = makeCell(IN);
  netBuilder.addCell(input2);

  const auto cell1 = makeCell(NAND, input1, input2);
  netBuilder.addCell(cell1);
  const auto output1 = makeCell(OUT, cell1);
  netBuilder.addCell(output1);

  const auto cell2 = makeCell(NOR, input1, input2);
  netBuilder.addCell(cell2);
  const auto output2 = makeCell(OUT, cell2);
  netBuilder.addCell(output2);

  const auto netID = netBuilder.make();
  const auto result = decomposeNet(netID);

  EXPECT_EQ(result.subnets.size(), 2);
}

TEST(NetDecomposerTest, CellReductionTest) {
  NetBuilder netBuilder;

  const auto input1 = makeCell(IN);
  netBuilder.addCell(input1);
  const auto input2 = makeCell(IN);
  netBuilder.addCell(input2);
  const auto input3 = makeCell(IN);
  netBuilder.addCell(input1);
  const auto input4 = makeCell(IN);
  netBuilder.addCell(input2);

  const auto cell1 = makeCell(AND, input1, input2);
  netBuilder.addCell(cell1);
  const auto cell2 = makeCell(AND, input3, input4);
  netBuilder.addCell(cell1);
  const auto cell3 = makeCell(AND, cell1, cell2);
  netBuilder.addCell(cell1);

  const auto output2 = makeCell(OUT, cell3);
  netBuilder.addCell(output2);

  const auto netID = netBuilder.make();

#ifdef UTOPIA_DEBUG
  std::cout << Net::get(netID) << std::endl;
#endif // UTOPIA_DEBUG

  NetDecomposer::Result result;
  NetDecomposer::get().decompose(netID, result);

#ifdef UTOPIA_DEBUG
  for (const auto &subnetInfo : result.subnets) {
    std::cout << Subnet::get(subnetInfo.subnetID) << std::endl;
  }
#endif // UTOPIA_DEBUG

  SubnetBuilder subnetBuilder;

  auto in1 = subnetBuilder.addInput();
  auto in2 = subnetBuilder.addInput();
  auto in3 = subnetBuilder.addInput();
  auto in4 = subnetBuilder.addInput();

  auto cell = subnetBuilder.addCell(AND, in1, in2, in3, in4);

  subnetBuilder.addOutput(cell);

  result.subnets[0].subnetID = subnetBuilder.make();

#ifdef UTOPIA_DEBUG
  const auto newNetID = NetDecomposer::get().compose(result);
  std::cout << Net::get(newNetID) << std::endl;
#endif // UTOPIA_DEBUG
}

TEST(NetDecomposerTest, SimpleNegTest) {
  NetBuilder netBuilder;

  const auto input1 = makeCell(IN);
  netBuilder.addCell(input1);
  const auto input2 = makeCell(IN);
  netBuilder.addCell(input2);

  const auto ninput1 = makeCell(NOT, input1);
  netBuilder.addCell(ninput1);
  const auto ninput2 = makeCell(NOT, input2);
  netBuilder.addCell(ninput2);

  const auto nninput1 = makeCell(NOT, ninput1);
  netBuilder.addCell(nninput1);
  const auto nninput2 = makeCell(NOT, ninput2);
  netBuilder.addCell(nninput2);

  const auto cell1 = makeCell(NAND, nninput1, nninput2);
  netBuilder.addCell(cell1);
  const auto output1 = makeCell(OUT, cell1);
  netBuilder.addCell(output1);

  const auto cell2 = makeCell(NOR, nninput1, nninput2);
  netBuilder.addCell(cell2);
  const auto output2 = makeCell(OUT, cell2);
  netBuilder.addCell(output2);

  const auto netID = netBuilder.make();
  const auto result = decomposeNet(netID);

  EXPECT_EQ(result.subnets.size(), 1);
}

TEST(NetDecomposerTest, LayerTest) {
  static constexpr size_t   nIn      = 32;
  static constexpr size_t   nOut     = 32;
  static constexpr size_t   nLayers  = 16;
  static constexpr size_t   minLayer = 2;
  static constexpr size_t   maxLayer = 16;
  static constexpr uint16_t minFanin = 1;
  static constexpr uint16_t maxFanin = 3;

  std::vector<CellSymbol> basis{ BUF, AND, OR, XOR, MAJ, NOT, NAND, NOR, XNOR };
  LayerGenerator generator(nIn, nOut, basis, nLayers, minLayer, maxLayer);
  generator.setFaninLim(minFanin, maxFanin);

  const auto netID = generator.generate();
  decomposeNet(netID);
}

} // namespace eda::gate::model
