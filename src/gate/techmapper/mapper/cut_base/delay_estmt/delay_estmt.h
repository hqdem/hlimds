//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <readcells/groups.h>

#include <string>
#include <vector>

namespace eda::gate::tech_optimizer::delay_estimation {

//===---------------------------------------------------------------------===//
// NLDM
//===---------------------------------------------------------------------===//

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
  NLDM() : delay(0.0), slew(0.0), capacitance(0.0) {};
  ~NLDM() = default;

  /// Getters
  float getCellDelay() {
    return delay;
  };

  float getCellCap() {
    return capacitance;
  };

  float getSlew() {
    return slew;
  };

  float lutInterpolation(const LookupTable *lut, size_t variablesCount, 
                         float& input_net_transition,
                         float& total_output_net_capacitance,
                         float& x1, float& x2, 
                         float& y1, float& y2, 
                         size_t& back1, size_t& front1, 
                         size_t& back2, size_t& front2);

  /*
   *  timingVisitor gets value of 
   *   concrete delay type from
   *   Timing's LookUp-Tables.
   */
  float timingVisitor(const Timing &timing,
                      std::string dtype,
                      float& input_net_transition,
                      float& total_output_net_capacitance);

  std::vector<float> timingVisitor(const Timing &timing,
                                   float& input_net_transition,
                                   float& total_output_net_capacitance);

  /*
   *  delayEstimation calls Parser to look for
   *  the concrete cell's timing values.
   */
  void delayEstimation(std::string& cell_name,
                       std::string& file_name,
                       float& input_net_transition,
                       float& total_output_net_capacitance);

  /*
   *  delayEstimation uses Library to look for
   *  the concrete cell's timing values.
   */
  void delayEstimation(std::string& cell_name,
                       Library& lib,
                       float& input_net_transition,
                       float& total_output_net_capacitance);

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

//===---------------------------------------------------------------------===//
// WLM
//===---------------------------------------------------------------------===//

/*
 *  Wire-load Model
 *  Class for delay estimation, based on such attributes as:
 *   Resistance, Capacitance, Slope
 */
class WLM {
  friend class NLDM;

public:
  WLM();
  WLM(std::string name);
  ~WLM() = default;

  // Setter
  void setWireLoadModel(std::string wlm_name);

  // Getters
  float getLength(size_t& fanout_count);
  float getFanoutCap(size_t& fanout_count);
  float getFanoutRes(size_t& fanout_count);

  // Properties
private:
  /* length_top = the length of one side of a square die
   * length_5k = the length of one side of a block containing
   * 5k gates
   */
  float length_sky = 23.2746;
  float length_5k = 1.7460;
  float length_3k = 1.5771;
  float length_1k = 1.3446;

  /*
   *  fudge = correction factor, routing, placement, etc.
   */
  float fudge = 1.0;

  /* WLM names = { "sky", "5k", "3k", "1k" }*/
  std::string wire_load_name;

  // Resistance, Capacitance, extrapolation slope
  float r, c, slope;
  std::pair<size_t, float> fanout_length[6];
  std::pair<size_t, float> fanout_resistance[6];
  std::pair<size_t, float> fanout_capacitance[6];
};

//===---------------------------------------------------------------------===//
// Delay estimator
//===---------------------------------------------------------------------===//
class DelayEstimator{
public:
  DelayEstimator() = default;
  ~DelayEstimator() = default;

  NLDM nldm;
  WLM wlm;
};

} // namespace eda::gate::tech_optimizer::delay_estimation
