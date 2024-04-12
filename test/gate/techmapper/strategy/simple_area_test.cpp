//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/techmapper/techmapper.h"

#include "gate/techmapper/techmapper_test_util.h"
#include "gate/techmapper/library/libertyManager.h"

#include "gtest/gtest.h"

namespace eda::gate::techmapper {

auto areaMapperType = Techmapper::MapperType::SIMPLE_AREA_FUNC;

TEST(TechmapAreaTest, SimpleAndSubnet) {
  LibraryManager::get().loadLibrary(libertyName);
  SubnetID mappedSubnetId = simpleANDMapping(areaMapperType);
  printResults(mappedSubnetId);
}

TEST(TechmapAreaTest, SimpleAndNotSubnet) {
  SubnetID mappedSubnetId = andNotMapping(areaMapperType);
  printResults(mappedSubnetId);
}

TEST(TechmapAreaTest, NotNotAndSubnet) {
  SubnetID mappedSubnetId = notNotAndMapping(areaMapperType);
  printResults(mappedSubnetId);
}

TEST(TechmapAreaTest, SimpleORSubnet) {
  SubnetID mappedSubnetId = simpleORMapping(areaMapperType);
  printResults(mappedSubnetId);
}

TEST(TechmapAreaTest, SimpleSubnet) {
  LibraryManager::get().loadLibrary(libertyName);
  SubnetID mappedSubnetId = andNotMapping(areaMapperType);
  printResults(mappedSubnetId);
}

TEST(TechmapAreaTest, GraphMLSubnet) {
  SubnetID mappedSubnetId = graphMLMapping(areaMapperType, "ss_pcm_orig");
  printResults(mappedSubnetId);
}

TEST(TechmapAreaTest, DISABLED_GraphMLSubnet2) {
  SubnetID mappedSubnetId = graphMLMapping(areaMapperType, "aes_orig");
  printResults(mappedSubnetId);
}

TEST(TechmapAreaTest, DISABLED_RandomSubnet) {
  SubnetID mappedSubnetId = randomMapping(areaMapperType);
  printResults(mappedSubnetId);
}
} // namespace eda::gate::techmapper
