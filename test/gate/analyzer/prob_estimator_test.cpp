//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/analyzer/probabilistic_estimate.h"
#include "gate/analyzer/simulation_estimator.h"
#include "gate/model2/subnet.h"

#include "gtest/gtest.h"

using CellSymbol    = eda::gate::model::CellSymbol;
using Link          = eda::gate::model::Subnet::Link;
using SimEstimator  = eda::gate::analyzer::SimulationEstimator;
using Subnet        = eda::gate::model::Subnet;
using SubnetBuilder = eda::gate::model::SubnetBuilder;
using ProbEstimator = eda::gate::analyzer::ProbabilisticEstimate;
using SwitchAct     = eda::gate::analyzer::SwitchActivity;

TEST(SwitchActivityTest, ProbabilisticEstimateTest) {

  SubnetBuilder subnetBuilder;
  const auto in = subnetBuilder.addInputs(4);
  
  const auto link1 = subnetBuilder.addCell(CellSymbol::OR, in[0], in[1]);
  const auto link2 = subnetBuilder.addCell(CellSymbol::AND, link1, in[2]);
  const auto link3 = subnetBuilder.addCell(CellSymbol::XOR, link2, in[3]);
  subnetBuilder.addOutput(link3);

  const auto &subnet = Subnet::get(subnetBuilder.make());
  
  ProbEstimator probEstimat;
  SwitchAct switchActProb = probEstimat.estimate(subnet);
  double allActivProb = switchActProb.getActivitySum();

  SimEstimator simEstimator;
  SwitchAct switchActSim = simEstimator.estimate(subnet);
  double allActivSim = switchActSim.getActivitySum();

  EXPECT_NEAR(allActivProb, allActivSim, 0.15);
}
