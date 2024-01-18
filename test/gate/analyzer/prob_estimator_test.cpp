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
  size_t in[4];
  for (size_t i{0}; i < 4; ++i) {
    in[i] = subnetBuilder.addInput();
  }
  size_t id{0};
  id = subnetBuilder.addCell(CellSymbol::OR, Link(in[0]), Link(in[1]));
  id = subnetBuilder.addCell(CellSymbol::AND, Link(id), Link(in[2]));
  id = subnetBuilder.addCell(CellSymbol::XOR, Link(id), Link(in[3]));
  subnetBuilder.addOutput(Link(id));

  const Subnet &subnet = Subnet::get(subnetBuilder.make());
  
  ProbEstimator probEstimat;
  SwitchAct switchActProb = probEstimat.estimate(subnet);
  double allActivProb = switchActProb.getActivitySum();

  SimEstimator simEstimator;
  SwitchAct switchActSim = simEstimator.estimate(subnet);
  double allActivSim = switchActSim.getActivitySum();

  EXPECT_NEAR(allActivProb, allActivSim, 0.1);
}