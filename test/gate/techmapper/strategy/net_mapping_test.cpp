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

// TODO Sequential mapper needs repairing
TEST(TechMapTest, DISABLED_SimpleNet) {
  NetID mappedNetId = simpleNetMapping(netMapperType);
  std::cout << mappedNetId;
  //printResults(mappedNetId);
}

} // namespace eda::gate::techmapper
