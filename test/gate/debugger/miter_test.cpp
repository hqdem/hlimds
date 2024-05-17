//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/base_checker.h"
#include "gate/model/examples.h"
#include "gate/simulator/simulator.h"

#include "gtest/gtest.h"

using namespace eda::gate::model;

namespace eda::gate::debugger {

TEST(MiterTest, Random) {
  using Simulator = eda::gate::simulator::Simulator;

  const size_t nIn      = 10;
  const size_t nOut     = 10;
  const size_t nCell    = 200;
  const size_t minArity = 2;
  const size_t maxArity = 7;
  const size_t nSubnet  = 40;

  for (size_t i = 0; i < nSubnet; ++i) {
    CellToCell map;
    auto id = makeSubnetRandomMatrix(nIn, nOut, nCell, minArity, maxArity, i);
    const Subnet &subnet = Subnet::get(id);

    for (size_t i = 0; i < subnet.getEntries().size(); ++i) {
      map[i] = i;
    }
    SubnetBuilder builder;
    BaseChecker::miter2(builder, id, id, map);
    const Subnet &miter = Subnet::get(builder.make());
    uint16_t miterInNum = miter.getInNum();
    EXPECT_TRUE(miter.getOutNum() == 1);
    EXPECT_TRUE(miterInNum == subnet.getInNum());

    Simulator simulator(miter);
    Simulator::DataVector values(miterInNum);

    for (size_t k = 0; k < miterInNum; ++k) {
      values[k] = std::rand();
    }
    simulator.simulate(values);
    EXPECT_FALSE(simulator.getValue(miter.getOut(0)));
  }

}
} // namespace eda::gate::debugger
