//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/techmapper/techmapper.h"
#include "gate/techmapper/techmapper_test_util.h"
#include "gate/techmapper/utils/get_statistics.h"

#include "gtest/gtest.h"

#define TYPE Techmapper::MapperType::AREA_FLOW

namespace eda::gate::techmapper {

TEST(TechmapAreaRecoveryTest, SubnetFromPaper) {
  SubnetID mappedSubnetId = areaRecoveySubnetMapping(TYPE);
  printStatistics(mappedSubnetId);
}

TEST(TechmapAreaRecoveryTest, GraphMLSubnet) {
  SubnetID mappedSubnetId = graphMLMapping(TYPE, "ss_pcm_orig");
  printStatistics(mappedSubnetId);
}

TEST(TechmapAreaRecoveryTest, DISABLED_GraphMLSubnet2) {
  SubnetID mappedSubnetId = graphMLMapping(TYPE, "aes_orig");
  printStatistics(mappedSubnetId);
}

TEST(TechmapAreaRecoveryTest, SimpleAndSubnet) {
  SubnetID mappedSubnetId = simpleANDMapping(TYPE);
  printStatistics(mappedSubnetId);
}

TEST(TechmapAreaRecoveryTest, SimpleORSubnet) {
  SubnetID mappedSubnetId = simpleORMapping(TYPE);
  printStatistics(mappedSubnetId);
}

TEST(TechmapAreaRecoveryTest, SimpleSubnet) {
  SubnetID mappedSubnetId = andNotMapping(TYPE);
  printStatistics(mappedSubnetId);
}

} // namespace eda::gate::techmapper
