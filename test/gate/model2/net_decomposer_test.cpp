//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021-2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/decomposer/net_decomposer.h"
#include "gate/model2/generator/layer_generator.h"
#include "gate/model2/printer/printer.h"

#include "gtest/gtest.h"

#include <cstdint>
#include <iostream>
#include <vector>

namespace eda::gate::model {

static std::vector<SubnetID> decomposeNet(NetID netID) {
  std::cout << Net::get(netID) << std::endl;

  const auto result = NetDecomposer::get().make(netID);
  for (const auto subnetID : result) {
    std::cout << Subnet::get(subnetID) << std::endl;
  }

  return result;
}

TEST(NetDecomposerTest, SimpleTest) {
  NetBuilder netBuilder;

  const auto input1 = makeCell(IN);
  netBuilder.addCell(input1);
  const auto input2 = makeCell(IN);
  netBuilder.addCell(input2);

  const auto cell1 = makeCell(AND, input1, input2);
  netBuilder.addCell(cell1);
  const auto output1 = makeCell(OUT, cell1);
  netBuilder.addCell(output1);

  const auto cell2 = makeCell(OR, input1, input2);
  netBuilder.addCell(cell2);
  const auto output2 = makeCell(OUT, cell2);
  netBuilder.addCell(output2);

  const auto netID = netBuilder.make();
  const auto result = decomposeNet(netID);

  EXPECT_EQ(result.size(), 2);
}

TEST(NetDecomposerTest, LayerTest) {
  static constexpr size_t   nIn      = 16;
  static constexpr size_t   nOut     = 16;
  static constexpr size_t   nLayers  = 8;
  static constexpr size_t   minLayer = 2;
  static constexpr size_t   maxLayer = 8;
  static constexpr uint16_t minFanin = 2;
  static constexpr uint16_t maxFanin = 3;

  std::vector<CellSymbol> basis{ AND, OR, XOR, MAJ };
  LayerGenerator generator(nIn, nOut, basis, nLayers, minLayer, maxLayer);
  generator.setFaninLim(minFanin, maxFanin);

  const auto netID = generator.generate();
  decomposeNet(netID);
}

} // namespace eda::gate::model
