//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gnet_test.h"
#include "gate/optimizer/bgnet.h"
#include "gate/optimizer/truthtable.h"

#include "gtest/gtest.h"

using namespace eda::gate::model;
using namespace eda::gate::optimizer;

bool areEquivalentTT(BoundGNet bgnet1, BoundGNet bgnet2) {
  return (TruthTable::build(bgnet1) == TruthTable::build(bgnet2));
}

bool cloneTest() {
  Gate::SignalList inputs;
  Gate::Id outputId;
  BoundGNet bnet;

  bnet.net = makeAnd(3, inputs, outputId);
  bnet.inputBindings = {inputs[0].node(), inputs[1].node(), inputs[2].node()};
  bnet.inputDelays = {1, 2, 3};

  BoundGNet newBnet = bnet.clone();
  return areEquivalentTT(bnet, newBnet) && (bnet.inputDelays == newBnet.inputDelays);
}

TEST(BoundGNetTest, CloneTest) {
  EXPECT_TRUE(cloneTest());
}
