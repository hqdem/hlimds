//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/sat_checker.h"
#include "gate/model2/utils/subnet_random.h"
#include "gate/model2/utils/subnet_truth_table.h"
#include "gate/optimizer/depth_optimizer.h"
#include "gate/optimizer/depth_replacer.h"
#include "gate/optimizer/depth_resynthesizer.h"
#include "gate/optimizer/depth_subnet_iterator.h"
#include "gate/optimizer/pass.h"
#include "gate/parser/graphml_test_utils.h"
#include "gate/parser/graphml_to_subnet.h"

#include "gtest/gtest.h"

#include <filesystem>

using namespace eda::gate::model;

using GraphMlSubnetParser = eda::gate::parser::graphml::GraphMlSubnetParser;
using ParserData          = GraphMlSubnetParser::ParserData;
using SatChecker          = eda::gate::debugger2::SatChecker;

namespace eda::gate::optimizer {

SubnetID parseGraphML(std::string fileName) {
  ParserData data;
  return eda::gate::parser::graphml::parse(fileName, data);
}

void checkEquivalence(std::string testName) {
  auto oldSubnetId = parseGraphML(testName);
  const auto &oldSubnet = Subnet::get(oldSubnetId);
  size_t depthBefore = oldSubnet.getPathLength().second;
  SubnetBuilder subnetBuilder;
  auto inputs = subnetBuilder.addInputs(oldSubnet.getInNum());
  auto outputs = subnetBuilder.addSubnet(oldSubnetId, inputs);
  subnetBuilder.addOutputs(outputs);

  DepthOptimizer rewritter(subnetBuilder, 15);
  rewritter.optimize();

  const auto &newSubnetId = subnetBuilder.make();
  const auto &newSubnet = Subnet::get(newSubnetId);
  size_t depthAfter = newSubnet.getPathLength().second;

  SatChecker &checker = SatChecker::get();

  std::unordered_map<size_t, size_t> map;
  for (size_t i{0}; i < oldSubnet.getInNum(); ++i) {
    map[i] = i;
  }

  for (int c = oldSubnet.getOutNum(); c > 0; --c) {
    map[oldSubnet.size() - c] = newSubnet.size() - c;
  }

  EXPECT_TRUE(depthAfter <= depthBefore);
  EXPECT_TRUE(checker.areEquivalent(oldSubnetId,
                                    newSubnetId, map).equal());
}

TEST(DepthOptimizerTest, sasc_orig) {
  checkEquivalence("sasc_orig");
}

TEST(DepthOptimizerTest, simple_spi_orig) {
  checkEquivalence("simple_spi_orig");
}

TEST(DepthOptimizerTest, usb_phy_orig) {
  checkEquivalence("usb_phy_orig");
}

TEST(DepthOptimizerTest, ss_pcm_orig) {
  checkEquivalence("ss_pcm_orig");
}

TEST(DepthOptimizerTest, i2c_orig) {
  checkEquivalence("i2c_orig");
}

} // namespace eda::gate::optimizer
