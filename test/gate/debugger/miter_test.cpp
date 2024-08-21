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

bool checkMiterCorrectness(const model::SubnetBuilder &miter) {
  EXPECT_TRUE(miter.getOutNum() == 1);

  simulator::Simulator simulator(miter);
  simulator::Simulator::DataVector values(miter.getInNum());

  for (size_t j = 0; j < miter.getInNum(); ++j) {
    values[j] = std::rand();
  }

  simulator.simulate(values);
  return !(simulator.getOutput(0));
}

TEST(MiterTest, Random) {
  const size_t nIn      = 10;
  const size_t nOut     = 10;
  const size_t nCell    = 10;
  const size_t minArity = 5;
  const size_t maxArity = 5;
  const size_t nSubnet  = 5;

  for (size_t i = 0; i < nSubnet; ++i) {
    const auto subnetBuilder = makeBuilderRandomMatrix(nIn, nOut, nCell,
                                                       minArity, maxArity, i);
    model::SubnetBuilder miter1;
    BaseChecker::makeMiter(miter1, *subnetBuilder, *subnetBuilder);
    EXPECT_TRUE(checkMiterCorrectness(miter1));

    const auto subnetID1 = makeSubnetRandomMatrix(nIn, nOut, nCell, minArity,
                                                  maxArity, i);
    model::SubnetBuilder miter2;
    BaseChecker::makeMiter(miter2, subnetID1, subnetID1);
    EXPECT_TRUE(checkMiterCorrectness(miter2));

    const auto subnetID2 = (*subnetBuilder).make();
    model::SubnetBuilder miter3;
    BaseChecker::makeMiter(miter3, subnetID2, subnetID2);
    EXPECT_TRUE(BaseChecker::getChecker(
                options::SAT).areEquivalent(
                miter1.make(), miter3.make()).equal());
  }
}

} // namespace eda::gate::debugger
