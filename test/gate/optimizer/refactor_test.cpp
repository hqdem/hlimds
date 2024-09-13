//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/sat_checker.h"
#include "gate/estimator/probabilistic_estimate.h"
#include "gate/model/subnet.h"
#include "gate/optimizer/pass.h"
#include "gate/translator/graphml_test_utils.h"

#include "gtest/gtest.h"

namespace eda::gate::optimizer {

using Estimator      = eda::gate::estimator::ProbabilityEstimator;
using Probabilities  = eda::gate::estimator::SwitchActivity::Probabilities;
using SatChecker     = eda::gate::debugger::SatChecker;
using Subnet         = eda::gate::model::Subnet;
using SubnetBuilder  = eda::gate::model::SubnetBuilder;
using SubnetID       = eda::gate::model::SubnetID;
using SubnetPass     = eda::gate::optimizer::SubnetPass;
using SwitchActivity = eda::gate::estimator::SwitchActivity;

void checkEquivalence(SubnetID source, SubnetID optimized) {
  SatChecker &checker = SatChecker::get();
  EXPECT_TRUE(checker.areEquivalent(source, optimized).equal()); 
}

SubnetID optimize(SubnetID source, SubnetPass &&pass) {
  auto builder = std::make_shared<SubnetBuilder>(source);
  pass->transform(builder);
  SubnetID optimized{builder->make(true)};
  checkEquivalence(source, optimized);
  return optimized;
}

void testRF(const std::string &design) {
  auto sourceID = eda::gate::translator::translateGmlOpenabc(design)->make();
  const auto &source = Subnet::get(sourceID);

  const auto &optimizedA = Subnet::get(optimize(sourceID, rf()));
  EXPECT_TRUE(optimizedA.size() <= source.size());

  const auto &optimizedD = Subnet::get(optimize(sourceID, rfd()));
  double sourceDepth = source.getPathLength().second;
  double optimizedDepth = optimizedD.getPathLength().second;
  EXPECT_TRUE(optimizedDepth <= sourceDepth);

  const auto optimizedP = optimize(sourceID, rfp());
  Estimator estimator;
  Probabilities probs;
  SwitchActivity activity;
  const auto sourceBuilder = std::make_shared<SubnetBuilder>(sourceID);
  const auto optimizedPBuilder = std::make_shared<SubnetBuilder>(optimizedP);
  estimator.estimate(sourceBuilder, probs, activity);
  double sourcePower = activity.getSwitchProbsSum();
  estimator.estimate(optimizedPBuilder, probs, activity);
  double optimizedPower = activity.getSwitchProbsSum();
  EXPECT_TRUE(optimizedPower <= sourcePower);
}

TEST(RefactorTest, sasc) {
  testRF("sasc_orig");
}

TEST(RefactorTest, ssPcm) {
  testRF("ss_pcm_orig");
}

TEST(RefactorTest, usbPhy) {
  testRF("usb_phy_orig");
}

} // namespace eda::gate::optimizer
