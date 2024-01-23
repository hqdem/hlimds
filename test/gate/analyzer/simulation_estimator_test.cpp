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
  const auto in = subnetBuilder.addInputs(4);

  const auto link1 = subnetBuilder.addCell(CellSymbol::OR,  in[0], in[1]);
  const auto link2 = subnetBuilder.addCell(CellSymbol::AND, link1, in[2]);
  const auto link3 = subnetBuilder.addCell(CellSymbol::XOR, link2, in[3]);
  subnetBuilder.addOutput(link3);

  const auto &subnet = Subnet::get(subnetBuilder.make());

  SimEstimator simEstimator;
  InValuesList data;

  // The input values for which the switches were calculated.
  data.push_back({0x327b23c66b8b4567, 0x66334873643c9869,
                  0x19495cff74b0dc51, 0x625558ec2ae8944a});

  data.push_back({0x46e87ccd238e1f29, 0x507ed7ab3d1b58ba,
                  0x41b71efb2eb141f2, 0x7545e14679e2a9e3});

  data.push_back({0x5bd062c2515f007c, 0x4db127f812200854,
                  0x1f16e9e80216231b, 0x66ef438d1190cde7});

  Switches preCalculatedSwitches{91, 95, 91, 106, 77, 83, 85, 85};

  Switches switches = simEstimator.countSwitches(subnet, data);

  EXPECT_EQ(switches, preCalculatedSwitches);
}
