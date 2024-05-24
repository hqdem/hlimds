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

#define TYPE Techmapper::Strategy::DELAY

namespace eda::gate::techmapper {

TEST(TechmapDelayTest, SimpleAndSubnet) {
  SubnetID mappedSubnetId = simpleANDMapping(TYPE);
  printStatistics(mappedSubnetId);
}

TEST(TechmapDelayTest, SimpleORSubnet) {
  SubnetID mappedSubnetId = simpleORMapping(TYPE);
  printStatistics(mappedSubnetId);
}

TEST(TechmapDelayTest, SimpleSubnet) {
  SubnetID mappedSubnetId = andNotMapping(TYPE);
  printStatistics(mappedSubnetId);
}

TEST(TechmapDelayTest, GraphMLSubnet) {
  SubnetID mappedSubnetId = graphMLMapping(TYPE, "ss_pcm_orig");
  printStatistics(mappedSubnetId);
}

TEST(TechmapDelayTest, DISABLED_GraphMLSubnet2) {
  SubnetID mappedSubnetId = graphMLMapping(TYPE, "aes_orig");
  printStatistics(mappedSubnetId);
}

TEST(TechmapDelayTest, DISABLED_RandomSubnet) {
  SubnetID mappedSubnetId = randomMapping(TYPE);
  printStatistics(mappedSubnetId);
}

} // namespace eda::gate::techmapper
