//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/subnet.h"
#include "gate/model2/utils/subnet_random.h"
#include "gate/model2/utils/subnet_truth_table.h"
#include "gate/optimizer/synthesis/akers.h"
#include "gate/optimizer/synthesizer.h"

#include "gtest/gtest.h"

using namespace eda::gate::model;
using namespace eda::gate::optimizer;

static kitty::dynamic_truth_table getTruthTable(SubnetID subnetID) {
  const auto &subnet = Subnet::get(subnetID);
  std::cout << subnet << std::endl;

  const auto table = evaluateSingleOut(subnet);
  std::cout << kitty::to_hex(table) << std::endl << std::endl;

  return table;
}

TEST(ResynthesizerTest, SimpleTest) {
  const size_t nIn      = 5;
  const size_t nOut     = 1;
  const size_t nCell    = 20;
  const size_t minArity = 2;
  const size_t maxArity = 3;
  const size_t nTest    = 2;

  for (size_t i = 0; i < nTest; ++i) {
    const auto oldID = randomSubnet(nIn, nOut, nCell, minArity, maxArity);
    const auto oldTable = getTruthTable(oldID);
  
    synthesis::AkersSynthesizer synthesizer;

    const auto newID = synthesizer.synthesize(oldTable);
    const auto newTable = getTruthTable(newID);

    EXPECT_TRUE(newTable == oldTable);
  }
}

