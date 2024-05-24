//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/design.h"
#include "gate/model/examples.h"

#include "gtest/gtest.h"

#include <cstddef>

namespace eda::gate::model {

inline void check(DesignBuilder &builder) {
  EXPECT_FALSE(builder.getSubnetNum() != 0);
  const auto netID = builder.make();
  EXPECT_TRUE(netID != OBJ_NULL_ID);

#ifdef UTOPIA_DEBUG
  std::cout << Net::get(netID) << std::endl;
#endif // UTOPIA_DEUB
}

TEST(DesignTest, RandomSubnet) {
  const size_t nIn = 10;
  const size_t nOut = 10;
  const size_t nCell = 30;
  const size_t minArity = 2;
  const size_t maxArity = 3;
  const unsigned seed = 0;

  const auto subnetID = makeSubnetRandomMatrix(
      nIn, nOut, nCell, minArity, maxArity, seed);

  DesignBuilder builder(subnetID);
  check(builder);
}

TEST(DesignTest, RandomNet) {
  const size_t nIn = 10;
  const size_t nOut = 10;
  const size_t nCell = 30;
  const size_t minArity = 2;
  const size_t maxArity = 3;
  const unsigned seed = 0;

  const auto netID = makeNetRandomMatrix(
      nIn, nOut, nCell, minArity, maxArity, seed);

  DesignBuilder builder(netID);
  check(builder);
}

} // namespace eda::gate::model
