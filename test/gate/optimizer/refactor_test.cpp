//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/analyzer/probabilistic_estimate.h"
#include "gate/debugger/sat_checker.h"
#include "gate/optimizer/pass.h"
#include "gate/optimizer/subnet_transformer.h"
#include "gate/parser/graphml_test_utils.h"

#include "gtest/gtest.h"

namespace eda::gate::optimizer {

using Estimator     = eda::gate::analyzer::ProbabilityEstimator;
using GraphMlParser = eda::gate::parser::graphml::GraphMlParser;
using SatChecker    = eda::gate::debugger::SatChecker;
using Subnet        = GraphMlParser::Subnet;
using SubnetBuilder = eda::gate::model::SubnetBuilder;
using SubnetID      = eda::gate::model::SubnetID;
using SubnetPass    = eda::gate::optimizer::SubnetPass;

void checkEquivalence(SubnetID source, SubnetID optimized) {
  SatChecker &checker = SatChecker::get();
  EXPECT_TRUE(checker.areEquivalent(source, optimized).equal()); 
}

const Subnet & optimize(SubnetID source, SubnetPass &&pass) {
  SubnetBuilder builder{source};
  pass->transform(builder);
  SubnetID optimized{builder.make(true)};
  checkEquivalence(source, optimized);
  return Subnet::get(optimized);
}

void testRF(const std::string &design) {
  SubnetID sourceID = parser::graphml::parse(design).make();
  const auto &source = Subnet::get(sourceID);
  
  const auto &optimizedA = optimize(sourceID, rf());
  EXPECT_TRUE(optimizedA.size() <= source.size());
  
  const auto &optimizedD = optimize(sourceID, rfd());
  double sourceDepth = source.getPathLength().second;
  double optimizedDepth = optimizedD.getPathLength().second;
  EXPECT_TRUE(optimizedDepth <= sourceDepth);

  const auto &optimizedP = optimize(sourceID, rfp());
  Estimator estimator;
  double sourcePower = estimator.estimate(source).getSwitchProbsSum();
  double optimizedPower = estimator.estimate(optimizedP).getSwitchProbsSum();
  EXPECT_TRUE(optimizedPower <= sourcePower);
}

TEST(RefactorTest, sasc) {
  testRF("sasc_orig.bench.graphml");
}

TEST(RefactorTest, ssPcm) {
  testRF("ss_pcm_orig.bench.graphml");
}

TEST(RefactorTest, usbPhy) {
  testRF("usb_phy_orig.bench.graphml");
}

} // namespace eda::gate::optimizer
