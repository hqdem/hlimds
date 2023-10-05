//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/examples.h"
#include "gate/optimizer/bgnet.h"
#include "gate/optimizer/truthtable.h"

#include "gtest/gtest.h"

namespace eda::gate::optimizer {

static bool areEquivalentTT(BoundGNet bgnet1, BoundGNet bgnet2) {
  return (TruthTable::build(bgnet1) == TruthTable::build(bgnet2));
}

bool cloneTest(BoundGNet bnet) {
  bnet.inputDelays = {1, 2, 3, 4, 5};

  BoundGNet newBnet = bnet.clone();
  return areEquivalentTT(bnet, newBnet) && (bnet.inputDelays == newBnet.inputDelays);
}

TEST(BoundGNetTest, CloneTest) {
  EXPECT_TRUE(cloneTest(makeTestBgnet1()));
  EXPECT_TRUE(cloneTest(makeTestBgnet2()));
  EXPECT_TRUE(cloneTest(makeTestBgnet3()));
}

} // namespace eda::gate::optimizer
