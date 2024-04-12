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

auto geneticMapperType = Techmapper::MapperType::GENETIC;

TEST(TechmapGeneticTest, SimpleAndSubnet) {
  SubnetID mappedSubnetId = simpleANDMapping(geneticMapperType);
  printResults(mappedSubnetId);
}

TEST(TechmapGeneticTest, SimpleORSubnet) {
  SubnetID mappedSubnetId = simpleORMapping(geneticMapperType);
  printResults(mappedSubnetId);
}

TEST(TechmapGeneticTest, SimpleSubnet) {
  SubnetID mappedSubnetId = andNotMapping(geneticMapperType);
  printResults(mappedSubnetId);
}

TEST(TechmapGeneticTest, GraphMLSubnet) {
  SubnetID mappedSubnetId = graphMLMapping(geneticMapperType, "ss_pcm_orig");
  printResults(mappedSubnetId);
}

TEST(TechmapGeneticTest, DISABLED_GraphMLSubnet2) {
  SubnetID mappedSubnetId = graphMLMapping(geneticMapperType, "aes_orig");
  printResults(mappedSubnetId);
}

TEST(TechmapGeneticTest, DISABLED_RandomSubnet) {
  SubnetID mappedSubnetId = randomMapping(geneticMapperType);
  printResults(mappedSubnetId);
}

} // namespace eda::gate::techmapper
