//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/seq_checker.h"
#include "gate/model/examples.h"
#include "gate/model/utils/subnet_random.h"

#include "gtest/gtest.h"

namespace eda::gate::debugger {

// FIXME:
#if 0
TEST(RegisterSweepTest, custom1) {
  const auto &subnet = model::Subnet::get(model::makeSubnetStuckLatches());
  const auto &clearedSubnet = structuralRegisterSweep(subnet, 10, false, 0);

  EXPECT_TRUE(clearedSubnet.size() == 3);
  EXPECT_TRUE(clearedSubnet.getInNum() == 0);
  EXPECT_TRUE(clearedSubnet.getOutNum() == 2);
  EXPECT_TRUE(clearedSubnet.getEntries()[0].cell.getSymbol() == model::ZERO);
}

TEST(RegisterSweepTest, custom2) {
  const auto &subnet = model::Subnet::get(model::makeSubnetStuckLatch());
  const auto &clearedSubnet = structuralRegisterSweep(subnet, 10, false, 0);

  const auto &entries = clearedSubnet.getEntries();

  EXPECT_TRUE(clearedSubnet.size() == 10);
  EXPECT_TRUE(clearedSubnet.getInNum() == 4);
  EXPECT_TRUE(clearedSubnet.getOutNum() == 5);
  EXPECT_FALSE(entries[0].cell.isFlipFlop() || entries[1].cell.isFlipFlop());
  EXPECT_TRUE(entries[2].cell.isFlipFlop() && entries[3].cell.isFlipFlop());
  EXPECT_TRUE(entries[4].cell.getSymbol() == model::ZERO);
  EXPECT_TRUE(entries[5].cell.isFlipFlop() && entries[6].cell.isFlipFlop());
  for (size_t i = 7; i <= 9; ++i) {
    EXPECT_FALSE(entries[i].cell.isFlipFlop());
  }
}
#endif

} // namespace eda::gate::debugger
