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

auto netMapperType = Techmapper::MapperType::SIMPLE_AREA_FUNC;

TEST(TechMapTest, SimpleNet) {
  NetID mappedSubnetId = simpleNetMapping(netMapperType);
  //printResults(mappedSubnetId);
}

} // namespace eda::gate::techmapper
