#include "gate/techoptimizer/mapper/cut_base/delay_estmt/delay_estmt.h"

#include "gtest/gtest.h"

#include <iostream>

using namespace eda::gate::tech_optimizer::delay_estimation;

TEST(DelayEstmt, secondTest) {
  DelayEstimator d1;
  d1.wlm.set_wire_load_model("2k");
  size_t fanout_count = 1;
  bool f1 = (d1.wlm.getLength(fanout_count) == 405);
  bool f2 = (d1.wlm.getFanoutCap(fanout_count) == 0.81000006198883057);
  EXPECT_TRUE((f1 == 1) && (f2 == 1));
  std::cout << "Length\tCap\tRes\n";
  for (size_t i = 1; i < 8; ++i)
    std::cout << d1.wlm.getLength(i) 
              << '\t' 
              << d1.wlm.getFanoutCap(i)
              << '\t'
              << d1.wlm.getFanoutRes(i)
              << std::endl;
}