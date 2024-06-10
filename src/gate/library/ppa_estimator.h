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

namespace eda::gate::library {

//===---------------------------------------------------------------------===//
// Structures, which contain data from LookUp-Tables
//===---------------------------------------------------------------------===//
/// Indexes from LUT
struct Ind {
  std::size_t back1 = 0, front1 = 0,
              back2 = 0, front2 = 0;
  int ind1 = -1, ind2 = -1;
};

/// Struct contains timingInfo from LUTS
struct DataTiming {
  std::vector<float> delayValues = {};
  std::size_t variablesCount = 7;
  bool interpolate = true;
  Ind index;
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
  NLDM(Library &library) : lib(library), delay(0.0),
           slew(0.0), capacitance(0.0),
           timingSense(0) {};
  virtual ~NLDM() = default;

  /// Getters
  float getCellDelay() const {
    return delay;
  };

  float getCellCap() const {
    return capacitance;
  };

  float getSlew() const {
    return slew;
  };

  int getSense() const {
    return timingSense;
  };

  float getLutValue(const std::vector<float>& lut_values,
                    const float inputNetTransition,
                    const float totalOutputNetCapacitance,
                    const float x1, const float x2,
                    const float y1, const float y2);

  float getLutValue(const LookupTable *lut,
                    const float inputNetTransition,
                    const float totalOutputNetCapacitance,
                    const float x1, const float x2,
                    const float y1, const float y2);

  void pinTimingEstimator(const Timing &timing,
                          const float inputNetTransition,
                          const float totalOutputNetCapacitance);

  void pinFTimingEstimator(const Timing &timing,
                           const float inputNetTransition,
                           const float totalOutputNetCapacitance);

  void pinITimingEstimator(const Timing &timing,
                           const float inputNetTransition,
                           const float totalOutputNetCapacitance);

  /*
   *  delayEstimation uses Library to look for
   *  the concrete cell's timing values.
   */
  void delayEstimation(const std::string &cellType,
                       const float inputNetTransition,
                       const float totalOutputNetCapacitance,
                       int &timingSense);

/// Properties
private:
  /// LookUp Table
  Library& lib;
  /// Data from LUT
  DataTiming context;
  /// cell delay
  float delay;
  /// transition delay
  float slew;
  /// cell capacitance
  float capacitance;
  /// Positive(0) or Negative(1) unate
  int timingSense;
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
  float getLength(const std::size_t& fanoutCount) const;
  float getFanoutCap(const std::size_t& fanoutCount) const;
  float getFanoutRes(const std::size_t& fanoutCount) const;

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
  std::pair<std::size_t, float> fanout_length[6];
  std::pair<std::size_t, float> fanout_resistance[6];
  std::pair<std::size_t, float> fanout_capacitance[6];
};

//===---------------------------------------------------------------------===//
// Delay estimator
//===---------------------------------------------------------------------===//
class DelayEstimator {
public:
  DelayEstimator(Library &library) : nldm(library) {};
  virtual ~DelayEstimator() = default;

  NLDM nldm;
  WLM wlm;
};

} // namespace eda::gate::library
