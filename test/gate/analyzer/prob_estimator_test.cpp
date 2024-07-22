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
#include "gate/optimizer/pass.h"
#include "gate/parser/graphml_test_utils.h"

#include "gtest/gtest.h"

using CellSymbol    = eda::gate::model::CellSymbol;
using Link          = eda::gate::model::Subnet::Link;
using LinkList      = eda::gate::model::Subnet::LinkList;
using Probabilities = eda::gate::analyzer::SwitchActivity::Probabilities;
using ProbEstimator = eda::gate::analyzer::ProbabilityEstimator;
using SimEstimator  = eda::gate::analyzer::SimulationEstimator;
using Subnet        = eda::gate::model::Subnet;
using SubnetID      = eda::gate::model::SubnetID;
using SubnetBuilder = eda::gate::model::SubnetBuilder;
using SwitchAct     = eda::gate::analyzer::SwitchActivity;

TEST(SwitchActivityTest, SubnetTest) {

  // Generating Subnet.
  SubnetBuilder builder;
  const auto in = builder.addInputs(6);
  LinkList links(5);
  links[0] = builder.addCell(CellSymbol::OR,  in[0], in[1]);
  links[1] = builder.addCell(CellSymbol::AND, links[0], in[2]);
  links[2] = builder.addCell(CellSymbol::XOR, links[1], in[3]);
  links[3] = builder.addCell(CellSymbol::AND, in[4], in[5]);
  links[4] = builder.addCell(CellSymbol::XOR, links[2], links[3]);
  builder.addOutput(links[4]);

  ProbEstimator probEstimat;
  SwitchAct switchActProb = probEstimat.estimate(builder);
  double allActivProb = switchActProb.getSwitchProbsSum();

  SimEstimator simEstimator;
  SwitchAct switchActSim = simEstimator.estimate(builder);
  double allActivSim = switchActSim.getSwitchProbsSum();

  EXPECT_NEAR(allActivProb, allActivSim, 0.5);

  Probabilities probs{0.2f, 0.3f, 0.1f, 0.4f, 0.6f, 0.7f};

  switchActProb = probEstimat.estimate(builder, probs);
  allActivProb = switchActProb.getSwitchProbsSum();

  switchActSim = simEstimator.estimate(builder, probs);
  allActivSim = switchActSim.getSwitchProbsSum();

  EXPECT_NEAR(allActivProb, allActivSim, 0.5);
}
