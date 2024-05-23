//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/analyzer/probabilistic_estimate.h"
#include "gate/analyzer/simulation_estimator.h"
#include "gate/model/subnet.h"

#include "gtest/gtest.h"

using CellSymbol    = eda::gate::model::CellSymbol;
using Link          = eda::gate::model::Subnet::Link;
using LinkList      = eda::gate::model::Subnet::LinkList;
using Probabilities = eda::gate::analyzer::SwitchActivity::Probabilities;
using ProbEstimator = eda::gate::analyzer::ProbabilisticEstimate;
using SimEstimator  = eda::gate::analyzer::SimulationEstimator;
using Subnet        = eda::gate::model::Subnet;
using SubnetBuilder = eda::gate::model::SubnetBuilder;
using SwitchAct     = eda::gate::analyzer::SwitchActivity;

TEST(SwitchActivityTest, ProbabilisticEstimateTest) {

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
  
  ProbEstimator probEstimat;
  SwitchAct switchActProb = probEstimat.estimate(subnet);
  double allActivProb = switchActProb.getSwitchProbabilitiesSum();

  SimEstimator simEstimator;
  SwitchAct switchActSim = simEstimator.estimate(subnet);
  double allActivSim = switchActSim.getSwitchProbabilitiesSum();

  EXPECT_NEAR(allActivProb, allActivSim, 0.5);

  Probabilities probs{0.2f, 0.3f, 0.1f, 0.4f, 0.6f, 0.7f};

  switchActProb = probEstimat.estimate(subnet, probs);
  allActivProb = switchActProb.getSwitchProbabilitiesSum();

  switchActSim = simEstimator.estimate(subnet, probs);
  allActivSim = switchActSim.getSwitchProbabilitiesSum();

  EXPECT_NEAR(allActivProb, allActivSim, 0.5);
}
