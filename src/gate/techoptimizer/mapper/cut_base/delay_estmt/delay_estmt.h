//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <iostream>
#include <vector>

namespace eda::gate::tech_optimizer::delay_estimation
{

  //===----------------------------------------------------------------------===//
  // NLDM
  //===----------------------------------------------------------------------===//

  /*
   *  Non-Linear Delay Model
   *  Basic class for delay estimation.
   */
  class NLDM {
  friend float interpolation(float x0, float y0,
                            float x1, float x2, 
                            float y1, float y2, 
                            float T11, float T12, 
                            float T21, float T22);

  public:
  /// Constructors
    NLDM() :
      delay(0.0), slew(0.0),
      capacitance(0.0) {};
    ~NLDM() = default;

  /// Getters
    float getCellDelay();
    float getSlew();
    /* 
    *  SkyWalker is calling Parser
    *  for searching concrete cell's timing.
    */
    void delayEstimation(std::string& cell_name,
                         const char* file_name,
                         float& input_net_transition_f,
                         float& total_output_net_capacitance_f );


  /// Properties
  private:
    /// cell delay
    float delay;
    /// transition delay
    float slew;
    /// cell capacitance
    float capacitance;
  };

  float interpolation(float x0, float y0,
                      float x1, float x2,
                      float y1, float y2,
                      float T11, float T12,
                      float T21, float T22);

  //===----------------------------------------------------------------------===//
  // WLM
  //===----------------------------------------------------------------------===//

  /*
   *  Wire-load Model
   *  Class for delay estimation, based on such attributes as:
   *   Resistance, Capacity, Area, Slope
   */
    class WLM
  {
    friend class NLDM;

  public:
    /// Constructors
    WLM();
    WLM(std::string name);
    ~WLM() = default;

    /// Setters
    void set_wire_load_model(std::string &wlm_name);

    /// Getters
    float getFanoutCap(int& fanout_count);

    /// Properties
  private:
    /* WLM names = { "top", "10k", "5k", "2k", "1k", "500" }*/
    std::string wire_load_name;
    /// Resistance, Capacitance, Area, extrapolation slope
    float R, C, Area, slope;
    std::pair<int, float> fanout_length[7];
    std::pair<int, float> fanout_resistance[7];
    std::pair<int, float> fanout_capacitance[7];
    /* length_top = the length of one side of a square die                *
     * length_10k = the length of one side of a block containing          *
     * 10k gates                                                          */
    float length_top = 2500.0;
    float length_10k = 900;
    /* fudge = correction factor, routing, placement, etc. */
    float fudge = 0.1;
  };

} // namespace eda::gate::tech_optimizer::delay_estimation
