//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/sat_checker.h"
#include "gate/model/design.h"
#include "gate/model/examples.h"
#include "gate/optimizer/pass.h"

#include "gtest/gtest.h"

#include <cstddef>

namespace eda::gate::model {

using namespace eda::gate::debugger;
using namespace eda::gate::optimizer;

static void test(DesignBuilder &builder) {
  EXPECT_TRUE(builder.getSubnetNum() != 0);

  foreach(aig())->transform(builder);
  builder.save("premapped");

  foreach(resyn())->transform(builder);
  builder.save("optimized");

  const auto &checker = SatChecker::get();
  const auto result = checker.areEquivalent(builder, "premapped", "optimized");
  EXPECT_TRUE(result.equal());

  const auto premappedNetID = builder.make("premapped");
  EXPECT_TRUE(premappedNetID != OBJ_NULL_ID);

  const auto optimizedNetID = builder.make("optimized");
  EXPECT_TRUE(optimizedNetID != OBJ_NULL_ID);

#ifdef UTOPIA_DEBUG
  std::cout << Net::get(premappedNetID) << std::endl;
  std::cout << Net::get(optimizedNetID) << std::endl;
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
  test(builder);
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
  test(builder);
}

} // namespace eda::gate::model
