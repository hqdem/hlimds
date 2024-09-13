//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/estimator/probabilistic_estimate.h"
#include "gate/estimator/simulation_estimator.h"
#include "gate/model/subnet.h"
#include "gate/optimizer/pass.h"

#include "gtest/gtest.h"

using CellSymbol    = eda::gate::model::CellSymbol;
using Link          = eda::gate::model::Subnet::Link;
using LinkList      = eda::gate::model::Subnet::LinkList;
using Probabilities = eda::gate::estimator::SwitchActivity::Probabilities;
using ProbEstimator = eda::gate::estimator::ProbabilityEstimator;
using SimEstimator  = eda::gate::estimator::SimulationEstimator;
using Subnet        = eda::gate::model::Subnet;
using SubnetID      = eda::gate::model::SubnetID;
using SubnetBuilder = eda::gate::model::SubnetBuilder;
using SwitchAct     = eda::gate::estimator::SwitchActivity;

TEST(SwitchActivityTest, SubnetTest) {

  // Generating Subnet.
  const auto builder = std::make_shared<SubnetBuilder>();
  const auto in = builder->addInputs(6);
  LinkList links(5);
  links[0] = builder->addCell(CellSymbol::OR,  in[0], in[1]);
  links[1] = builder->addCell(CellSymbol::AND, links[0], in[2]);
  links[2] = builder->addCell(CellSymbol::XOR, links[1], in[3]);
  links[3] = builder->addCell(CellSymbol::AND, in[4], in[5]);
  links[4] = builder->addCell(CellSymbol::XOR, links[2], links[3]);
  builder->addOutput(links[4]);

  ProbEstimator probEstimator;
  Probabilities probs;
  SwitchAct switchActProb;
  probEstimator.estimate(builder, probs, switchActProb);
  double allActivProb = switchActProb.getSwitchProbsSum();

  SimEstimator simEstimator;
  SwitchAct switchActSim;
  simEstimator.estimate(builder, probs, switchActSim);
  double allActivSim = switchActSim.getSwitchProbsSum();

  EXPECT_NEAR(allActivProb, allActivSim, 0.5);

  probs = {0.2f, 0.3f, 0.1f, 0.4f, 0.6f, 0.7f};

  probEstimator.estimate(builder, probs, switchActProb);
  allActivProb = switchActProb.getSwitchProbsSum();

  simEstimator.estimate(builder, probs, switchActSim);
  allActivSim = switchActSim.getSwitchProbsSum();

  EXPECT_NEAR(allActivProb, allActivSim, 0.5);
}
