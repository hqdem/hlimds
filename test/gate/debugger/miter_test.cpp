//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/base_checker.h"
#include "gate/model/examples.h"
#include "gate/simulator/simulator.h"

#include "gtest/gtest.h"

using namespace eda::gate::model;

namespace eda::gate::debugger {

TEST(MiterTest, Random) {
  const size_t nIn      = 10;
  const size_t nOut     = 10;
  const size_t nCell    = 200;
  const size_t minArity = 2;
  const size_t maxArity = 7;
  const size_t nSubnet  = 40;

  for (size_t i = 0; i < nSubnet; ++i) {
    const auto subnetID = makeSubnetRandomMatrix(
        nIn, nOut, nCell, minArity, maxArity, i);

    model::SubnetBuilder miter;
    BaseChecker::makeMiter(miter, subnetID, subnetID);

    EXPECT_TRUE(miter.getOutNum() == 1);

    simulator::Simulator simulator(miter);
    simulator::Simulator::DataVector values(miter.getInNum());

    for (size_t j = 0; j < miter.getInNum(); ++j) {
      values[j] = std::rand();
    }

    simulator.simulate(values);
    EXPECT_FALSE(simulator.getOutput(0));
  }
}

} // namespace eda::gate::debugger
