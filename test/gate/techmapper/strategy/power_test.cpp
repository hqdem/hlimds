//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/techmapper/techmapper.h"

#include "gate/techmapper/techmapper_test_util.h"
#include "gate/techmapper/library/liberty_manager.h"

#include "gtest/gtest.h"

namespace eda::gate::techmapper {

auto powerMapperType = Techmapper::MapperType::POWER;

TEST(TechmapPowerTest, SimpleAndSubnet) {
  SubnetID mappedSubnetId = simpleANDMapping(powerMapperType);
  printResults(mappedSubnetId);
}

TEST(TechmapPowerTest, SimpleAndNotSubnet) {
  SubnetID mappedSubnetId = andNotMapping(powerMapperType);
  printResults(mappedSubnetId);
}

// TEST(TechmapPowerTest, OKSimpleAndNotSubnet) {
//   SubnetID mappedSubnetId = OKandNotMapping(powerMapperType);
//   printResults(mappedSubnetId);
// }

TEST(TechmapPowerTest, NotNotAndSubnet) {
  SubnetID mappedSubnetId = notNotAndMapping(powerMapperType);
  printResults(mappedSubnetId);
}

TEST(TechmapPowerTest, SimpleORSubnet) {
  SubnetID mappedSubnetId = simpleORMapping(powerMapperType);
  printResults(mappedSubnetId);
}

TEST(TechmapPowerTest, SimpleSubnet) {
  SubnetID mappedSubnetId = andNotMapping(powerMapperType);
  printResults(mappedSubnetId);
}

TEST(TechmapPowerTest, GraphMLSubnet) {
  SubnetID mappedSubnetId = graphMLMapping(powerMapperType, "ss_pcm_orig");
  printResults(mappedSubnetId);
}

TEST(TechmapPowerTest, DISABLED_GraphMLSubnet2) {
  SubnetID mappedSubnetId = graphMLMapping(powerMapperType, "aes_orig");
  printResults(mappedSubnetId);
}

TEST(TechmapPowerTest, DISABLED_RandomSubnet) {
  SubnetID mappedSubnetId = randomMapping(powerMapperType);
  printResults(mappedSubnetId);
}
} // namespace eda::gate::techmapper
