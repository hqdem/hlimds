//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/subnet.h"
#include "gate/model2/utils/subnet_random.h"
#include "gate/simulator2/simulator.h"

#include "gtest/gtest.h"

using namespace eda::gate::model;
using namespace eda::gate::simulator2;

TEST(SimulatorTest, SimpleTest) {
  const size_t nIn      = 5;
  const size_t nOut     = 1;
  const size_t nCell    = 20;
  const size_t minArity = 2;
  const size_t maxArity = 3;
  const size_t nSubnet  = 2;
  const size_t nTest    = 100;

  for (size_t i = 0; i < nSubnet; ++i) {
    const auto subnetID = randomSubnet(nIn, nOut, nCell, minArity, maxArity);

    const auto &subnet = Subnet::get(subnetID);
    std::cout << subnet << std::endl;

    Simulator simulator(subnet);
    Simulator::DV values(nIn);

    for (size_t j = 0; j < nTest; ++j) {
      for (size_t k = 0; k < nIn; ++k) {
        values[k] = std::rand();
      }    

      simulator.simulate(values);
    }
  }
}

