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

#define TYPE Techmapper::Strategy::GENETIC

namespace eda::gate::techmapper {

TEST(TechmapGeneticTest, DISABLED_SimpleAndSubnet) {
  SubnetID mappedSubnetId = simpleANDMapping(TYPE);
  printStatistics(mappedSubnetId);
}

TEST(TechmapGeneticTest, DISABLED_SimpleORSubnet) {
  SubnetID mappedSubnetId = simpleORMapping(TYPE);
  printStatistics(mappedSubnetId);
}

TEST(TechmapGeneticTest, DISABLED_SimpleSubnet) {
  SubnetID mappedSubnetId = andNotMapping(TYPE);
  printStatistics(mappedSubnetId);
}

TEST(TechmapGeneticTest, DISABLED_GraphMLSubnet) {
  SubnetID mappedSubnetId = graphMLMapping(TYPE, "ss_pcm_orig");
  printStatistics(mappedSubnetId);
}

TEST(TechmapGeneticTest, DISABLED_GraphMLSubnet2) {
  SubnetID mappedSubnetId = graphMLMapping(TYPE, "aes_orig");
  printStatistics(mappedSubnetId);
}

TEST(TechmapGeneticTest, DISABLED_RandomSubnet) {
  SubnetID mappedSubnetId = randomMapping(TYPE);
  printStatistics(mappedSubnetId);
}

} // namespace eda::gate::techmapper
