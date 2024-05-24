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

#define TYPE Techmapper::Strategy::AREA

namespace eda::gate::techmapper {

// TODO Sequential mapper needs repairing
TEST(TechmapTest, DISABLED_SimpleNet) {
  NetID mappedNetId = simpleNetMapping(TYPE);
  std::cout << mappedNetId;
  //printStatistics(mappedNetId);
}

} // namespace eda::gate::techmapper
