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
  double allActivProb = switchActProb.getSwitchProbsSum();

  SimEstimator simEstimator;
  SwitchAct switchActSim = simEstimator.estimate(subnet);
  double allActivSim = switchActSim.getSwitchProbsSum();

  EXPECT_NEAR(allActivProb, allActivSim, 0.5);

  Probabilities probs{0.2f, 0.3f, 0.1f, 0.4f, 0.6f, 0.7f};

  switchActProb = probEstimat.estimate(subnet, probs);
  allActivProb = switchActProb.getSwitchProbsSum();

  switchActSim = simEstimator.estimate(subnet, probs);
  allActivSim = switchActSim.getSwitchProbsSum();

  EXPECT_NEAR(allActivProb, allActivSim, 0.5);
}

TEST(SwitchActivityTest, SubnetBuilderTest) {

  SubnetBuilder builder = 
      eda::gate::parser::graphml::parse("des3_area_orig.bench.graphml");

  const auto &pass = eda::gate::optimizer::rw();
  pass->transform(builder);

  ProbEstimator estimator;
  auto builderRes = estimator.estimateProbs(builder);
  
  std::vector<size_t> map;
  map.resize(*(--builder.end()) + 1);
  for (auto it{builder.begin()}; it != builder.end(); ++it) {
    map[*it] = *it;
  }

  SubnetID subnetID = builder.make(map);
  const auto &subnet = Subnet::get(subnetID);
  auto subnetRes = estimator.estimateProbs(subnet);

  const double exp = 1e-6;

  for (size_t i{0}; i < subnetRes.size(); ++i) {
    EXPECT_NEAR(subnetRes[i], builderRes[map[i]], exp);
  }
}