//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/analyzer/simulation_estimator.h"
#include "gate/model2/subnet.h"

#include "gtest/gtest.h"

using CellSymbol    = eda::gate::model::CellSymbol;
using InValuesList  = eda::gate::analyzer::SimulationEstimator::InValuesList;
using Link          = eda::gate::model::Subnet::Link;
using LinkList      = eda::gate::model::Subnet::LinkList;
using SimEstimator  = eda::gate::analyzer::SimulationEstimator;
using Subnet        = eda::gate::model::Subnet;
using SubnetBuilder = eda::gate::model::SubnetBuilder;
using Switches      = eda::gate::analyzer::SimulationEstimator::Switches;

TEST(SwitchActivityTest, ToggleRateTest) {
  // Generating Subnet.
  SubnetBuilder subnetBuilder;
  const auto in = subnetBuilder.addInputs(6);
  LinkList links(5);
  links[0] = subnetBuilder.addCell(CellSymbol::OR,  in[0], in[1]);
  links[1] = subnetBuilder.addCell(CellSymbol::AND, links[0], in[2]);
  links[2] = subnetBuilder.addCell(CellSymbol::XOR, links[1], in[3]);
  links[3] = subnetBuilder.addCell(CellSymbol::AND, in[4], in[5]);
  links[4] = subnetBuilder.addCell(CellSymbol::XOR, links[2], links[3]);
  subnetBuilder.addOutput(links[4]);

  const auto &subnet = Subnet::get(subnetBuilder.make());

  SimEstimator simEstimator;
  InValuesList data;

  // The input values for which the switches were calculated.
  data.push_back({0x327b23c66b8b4567, 0x66334873643c9869,
                  0x19495cff74b0dc51, 0x625558ec2ae8944a,
                  0xeede4b96a8d1befe, 0x21a9a65a32528163});

  data.push_back({0x46e87ccd238e1f29, 0x507ed7ab3d1b58ba,
                  0x41b71efb2eb141f2, 0x7545e14679e2a9e3,
                  0xeede4bb14902d781, 0xa2a88011eede4b11});

  data.push_back({0x5bd062c2515f007c, 0x4db127f812200854,
                  0x1f16e9e80216231b, 0x66ef438d1190cde7,
                  0x0527016b14902d78, 0xa2a880118b0821a1});

  auto [switchesOn, switchesOff] = simEstimator.countSwitches(subnet, data);

  Switches
      preCalculatedSwitchesOn{45, 47, 45, 53, 49, 50, 38, 41, 42, 30, 44, 44};
  Switches
      preCalculatedSwitchesOff{46, 48, 46, 53, 49, 50, 39, 42, 43, 30, 45, 45};

  EXPECT_EQ(switchesOn, preCalculatedSwitchesOn);
  EXPECT_EQ(switchesOff, preCalculatedSwitchesOff);
}
