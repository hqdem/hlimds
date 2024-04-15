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

auto delayMapperType = Techmapper::MapperType::SIMPLE_DELAY_FUNC;

TEST(TechmapDelayTest, SimpleAndSubnet) {
  SubnetID mappedSubnetId = simpleANDMapping(delayMapperType);
  printResults(mappedSubnetId);
}

TEST(TechmapDelayTest, SimpleORSubnet) {
  SubnetID mappedSubnetId = simpleORMapping(delayMapperType);
  printResults(mappedSubnetId);
}

TEST(TechmapDelayTest, SimpleSubnet) {
  SubnetID mappedSubnetId = andNotMapping(delayMapperType);
  printResults(mappedSubnetId);
}

TEST(TechmapDelayTest, GraphMLSubnet) {
  SubnetID mappedSubnetId = graphMLMapping(delayMapperType, "ss_pcm_orig");
  printResults(mappedSubnetId);
}

TEST(TechmapDelayTest, DISABLED_GraphMLSubnet2) {
  SubnetID mappedSubnetId = graphMLMapping(delayMapperType, "aes_orig");
  printResults(mappedSubnetId);
}

TEST(TechmapDelayTest, DISABLED_RandomSubnet) {
  SubnetID mappedSubnetId = randomMapping(delayMapperType);
  printResults(mappedSubnetId);
}

} // namespace eda::gate::techmapper
