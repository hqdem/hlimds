#include "gate/techoptimizer/mapper/cut_base/delay_estmt/delay_estmt.h"

#include "gtest/gtest.h"

#include <iostream>

using namespace eda::gate::tech_optimizer::delay_estimation;

TEST(DelayEstmt, firstTest) {
  std::string file = "test/data/gate/tech_mapper/sky130_fd_sc_hd__ff_100C_1v65.lib";
  
  DelayEstimator d1;
  DelayEstimator d2;
  DelayEstimator d3;
  
  std::string cell_name1 = "sky130_fd_sc_hd__a2111o_4";
  float input_net_transition = 0.053133;
  float total_output_net_capacitance1 = 0.191204;
  d1.nldm.delayEstimation(cell_name1, file, input_net_transition, total_output_net_capacitance1);
  float delay1 = 0.4451143741607666;
  float slew1 = 0.37748932838439941;
  EXPECT_DOUBLE_EQ(d1.nldm.getCellDelay(), delay1);
  EXPECT_DOUBLE_EQ(d1.nldm.getSlew(), slew1);

  std::string cell_name2 = "sky130_fd_sc_hd__o21a_4";
  float total_output_net_capacitance2 = 0.001627;
  d2.nldm.delayEstimation(cell_name2, file, input_net_transition, total_output_net_capacitance2);
  float delay2 = 0.09489276260137558;
  float slew2 = 0.024740446358919144;
  EXPECT_DOUBLE_EQ(d2.nldm.getCellDelay(), delay2);
  EXPECT_DOUBLE_EQ(d2.nldm.getSlew(), slew2);

  std::string cell_name3 = "sky130_fd_sc_hd__a211o_2";
  float input_net_transition3 = 0.099999;
  float total_output_net_capacitance3 = 0.002468;
  d3.nldm.delayEstimation(cell_name3, file, input_net_transition3, total_output_net_capacitance3);
  float delay3 = 0.11193791031837463;
  float slew3 = 0.034578997641801834;
  EXPECT_DOUBLE_EQ(d3.nldm.getCellDelay(), delay3);
  EXPECT_DOUBLE_EQ(d3.nldm.getSlew(), slew3);
}
