//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/library/library_types.h"

#include <string>
#include <vector>

namespace eda::gate::estimator {

//===---------------------------------------------------------------------===//
// Structures, which contain data from LookUp-Tables
//===---------------------------------------------------------------------===//
/// Indexes from LUT
struct Index {
  std::size_t back1 = 0, front1 = 0,
              back2 = 0, front2 = 0;
  int ind1 = -1, ind2 = -1;
};

/// Struct contains timingInfo from LUTS
struct DataTimingContext {
  std::vector<double> delayValues = {};
  std::size_t variablesCount = 7;
  bool interpolate = true;
  Index index;
  double inputTransTime;
  double outputTotalCap;
  DataTimingContext(double inputTransTime, double outputTotalCap) :
    inputTransTime(inputTransTime), outputTotalCap(outputTotalCap) {}
};

//===---------------------------------------------------------------------===//
// NLDM
//===---------------------------------------------------------------------===//

/*
 *  Non-Linear Delay Model
 *  Basic class for delay estimation.
 */
class NLDM {
public:
  virtual ~NLDM() = default;

private:
  static double getLutValue(const std::vector<double>& lut_values,
                    DataTimingContext &context,
                    const double x1, const double x2,
                    const double y1, const double y2);

  static void pinTimingEstimator(const std::vector<const library::LUT*> &luts,
                          DataTimingContext &context);

  static void pinFTimingEstimator(const std::vector<const library::LUT*> &luts,
                           DataTimingContext &context);

  static void pinITimingEstimator(const std::vector<const library::LUT*> &luts,
                           DataTimingContext &context);

public:
  /*
   *  delayEstimation uses Library to look for
   *  the concrete cell's timing values.
   *  timingSence - Positive or Negative unate (rise or fall)
   *  slew - ??
   *  delay - ??
   */
  static void delayEstimation(
    const library::StandardCell &cell,
    const double inputTransTime,
    const double outputTotalCap,
    int &timingSense, double &slew, double &delay, double &cap);

  static double delayEstimation(
    const library::StandardCell &cell,
    const double inputTransTime,
    const double outputTotalCap);
};

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
  WLM(const std::string &name);
  virtual ~WLM() = default;

  // Setter
  void setWireLoadModel(const std::string &wlm_name);

  // Getters
  double getLength(const std::size_t& fanoutCount) const;
  double getFanoutCap(const std::size_t& fanoutCount) const;
  double getFanoutRes(const std::size_t& fanoutCount) const;

  // Properties
private:
  /* length_top = the length of one side of a square die
   * length_5k = the length of one side of a block containing
   * 5k gates
   */
  double length_sky = 23.2746;
  double length_5k = 1.7460;
  double length_3k = 1.5771;
  double length_1k = 1.3446;

  /*
   *  fudge = correction factor, routing, placement, etc.
   */
  double fudge = 1.0;

  /* WLM names = { "sky", "5k", "3k", "1k" }*/
  std::string wire_load_name;

  // Resistance, Capacitance, extrapolation slope
  double r, c, slope;
  std::pair<std::size_t, double> fanout_length[6];
  std::pair<std::size_t, double> fanout_resistance[6];
  std::pair<std::size_t, double> fanout_capacitance[6];
};

//===---------------------------------------------------------------------===//
// Delay estimator
//===---------------------------------------------------------------------===//
class DelayEstimator {
public:
  virtual ~DelayEstimator() = default;

  NLDM nldm;
  WLM wlm;
};

} // namespace eda::gate::estimator
