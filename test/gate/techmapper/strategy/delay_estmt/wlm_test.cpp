//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/techmapper/comb_mapper/cut_based/delay_estmt/delay_estmt.h"

#include "gtest/gtest.h"

#include <iostream>

namespace eda::gate::techmapper {

TEST(DelayEstmt, secondTest) {
  DelayEstimator d1;
  size_t fanout_count = 1;
  bool f1 = (d1.wlm.getLength(fanout_count) == 23.274599075317383);
  bool f2 = (d1.wlm.getFanoutCap(fanout_count) == 0.00046549196122214198);

  EXPECT_TRUE((f1 == 1) && (f2 == 1));

  std::cout << "Length\tCap\tRes\n";
  for (size_t i = 1; i < 6; ++i)
    std::cout << d1.wlm.getLength(i)
              << '\t'
              << d1.wlm.getFanoutCap(i)
              << '\t'
              << d1.wlm.getFanoutRes(i)
              << std::endl;
}

} // namespace eda::gate::techmapper
