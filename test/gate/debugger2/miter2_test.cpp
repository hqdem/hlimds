//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger2/rnd_checker2.h"
#include "gate/model2/utils/subnet_random.h"

#include "gtest/gtest.h"

using namespace eda::gate::model;

namespace eda::gate::debugger2 {

TEST(MiterTest, Random) {
  using Simulator = eda::gate::simulator2::Simulator;
  using options::RND;

  const size_t nIn      = 10;
  const size_t nOut     = 10;
  const size_t nCell    = 200;
  const size_t minArity = 2;
  const size_t maxArity = 7;
  const size_t nSubnet  = 100;

  for (size_t i = 0; i < nSubnet; ++i) {
    auto subnetID = randomSubnet(nIn, nOut, nCell, minArity, maxArity);
    auto &subnet = Subnet::get(subnetID);

    CellToCell map;
    for (size_t i = 0; i < subnet.getEntries().size(); ++i) {
      map[i] = i;
    }
    const auto &miter = miter2(subnet, subnet, map);
    EXPECT_TRUE(miter.getOutNum() == 1);
    EXPECT_TRUE(miter.getInNum() == subnet.getInNum());

    Simulator simulator(miter);
    Simulator::DataVector values(nIn);

    for (size_t k = 0; k < nIn; ++k) {
      values[k] = std::rand();
    }
    simulator.simulate(values);
    EXPECT_FALSE(simulator.getValue(miter.getOut(0)));

    static_cast<RndChecker2&>(getChecker(RND)).setExhaustive(false);
    static_cast<RndChecker2&>(getChecker(RND)).setTries(100);
    EXPECT_TRUE(getChecker(RND).equivalent(subnet, subnet, map).isUnknown());

    static_cast<RndChecker2&>(getChecker(RND)).setExhaustive(true);
    EXPECT_TRUE(getChecker(RND).equivalent(subnet, subnet, map).equal());
  }
}
} // namespace eda::gate::debugger2
