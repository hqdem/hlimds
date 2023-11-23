//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger2/rnd_checker2.h"
#include "gate/model2/utils/subnet_random.h"

#include "gtest/gtest.h"

using namespace eda::gate::model;

namespace eda::gate::debugger2 {

TEST(MiterTest, Random) {
  const size_t nIn      = 10;
  const size_t nOut     = 10;
  const size_t nCell    = 200;
  const size_t minArity = 2;
  const size_t maxArity = 7;
  const size_t nSubnet  = 100;

  for (size_t i = 0; i < nSubnet; ++i) {
    Subnet s = Subnet::get(randomSubnet(nIn, nOut, nCell, minArity, maxArity));
    CellToCell map;
    for (size_t i = 0; i < s.getEntries().size(); ++i) {
      map[i] = i;
    }
    MiterHints hints = makeHints(s, map);
    Subnet miter = miter2(s, s, hints);
    EXPECT_TRUE(miter.getOutNum() == 1);
    EXPECT_TRUE(miter.getInNum() == s.getInNum());
    eda::gate::simulator2::Simulator simulator(miter);
    eda::gate::simulator2::Simulator::DV values(nIn);
    for (size_t k = 0; k < nIn; ++k) {
      values[k] = std::rand();
    }
    simulator.simulate(values);
    EXPECT_FALSE(simulator.getValue(miter.getEntries().size() - 1)) << std::endl;
    RndChecker2 rnd(false, 100);
    EXPECT_TRUE(rnd.equivalent(s, s, map).isUnknown());
    rnd.setExhaustive(true);
    EXPECT_TRUE(rnd.equivalent(s, s, map).equal());
  }
}
} // namespace eda::gate::debugger2
