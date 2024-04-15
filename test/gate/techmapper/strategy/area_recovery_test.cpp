//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/techmapper/techmapper.h"
#include "gate/techmapper/techmapper_test_util.h"

#include "gtest/gtest.h"

namespace eda::gate::techmapper {

auto areaRecoveryMapperType = Techmapper::MapperType::AREA_FLOW;

TEST(TechmapAreaRecoveryTest, SubnetFromPaper) {
  SubnetID mappedSubnetId = areaRecoveySubnetMapping(areaRecoveryMapperType);
  printResults(mappedSubnetId);
}

TEST(TechmapAreaRecoveryTest, GraphMLSubnet) {
  SubnetID mappedSubnetId = graphMLMapping(areaRecoveryMapperType, "ss_pcm_orig");
  printResults(mappedSubnetId);
}

TEST(TechmapAreaRecoveryTest, DISABLED_GraphMLSubnet2) {
  SubnetID mappedSubnetId = graphMLMapping(areaRecoveryMapperType, "aes_orig");
  printResults(mappedSubnetId);
}

TEST(TechmapAreaRecoveryTest, SimpleAndSubnet) {
  SubnetID mappedSubnetId = simpleANDMapping(areaRecoveryMapperType);
  printResults(mappedSubnetId);
}

TEST(TechmapAreaRecoveryTest, SimpleORSubnet) {
  SubnetID mappedSubnetId = simpleORMapping(areaRecoveryMapperType);
  printResults(mappedSubnetId);
}

TEST(TechmapAreaRecoveryTest, SimpleSubnet) {
  SubnetID mappedSubnetId = andNotMapping(areaRecoveryMapperType);
  printResults(mappedSubnetId);
}

} // namespace eda::gate::techmapper
