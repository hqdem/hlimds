//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/subnet.h"
#include "gate/model/utils/subnet_random.h"
#include "gate/simulator/simulator.h"

#include "gtest/gtest.h"

namespace eda::gate::simulator {

TEST(SimulatorTest, SimpleTest) {
  constexpr size_t nIn      = 5;
  constexpr size_t nOut     = 1;
  constexpr size_t nCell    = 20;
  constexpr size_t minArity = 2;
  constexpr size_t maxArity = 3;
  constexpr size_t nSubnet  = 1;
  constexpr size_t nTest    = 1;

  for (size_t i = 0; i < nSubnet; ++i) {
    const auto id = model::randomSubnet(nIn, nOut, nCell, minArity, maxArity);
    auto builder = std::make_shared<model::SubnetBuilder>(id);

    Simulator simulator(builder);
    Simulator::DataVector values(nIn);

    for (size_t j = 0; j < nTest; ++j) {
      for (size_t k = 0; k < nIn; ++k) {
        values[k] = std::rand();
        values[k] = values[k] << 32 | std::rand();
      }    

      simulator.simulate(values);
    }
  }
}

} // namespace eda::gate::simulator
