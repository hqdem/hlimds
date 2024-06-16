//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/estimator/simple_time_model.h"
#include "util/env.h"

#include <readcells/ast.h>
#include <readcells/ast_parser.h>
#include <readcells/groups.h>
#include <readcells/token_parser.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace eda::gate::estimator {

WLM::WLM() :
  wire_load_name("sky"),
  r(fudge * 0.08), c(fudge * 0.0002),
  slope(8.3631) {
  /* Capacitance is 0.02ff/micron for avg metal         */
  /* Resistance is 80 m-ohm/square, in kohm units     */
  /* (remember that our capacitance unit is 1.0pf)     */

  setWireLoadModel(wire_load_name);
}

WLM::WLM(const std::string &name) :
  r(fudge * 0.004), c(fudge * 0.2),
  slope(6.2836) {

  setWireLoadModel(name);
}

void WLM::setWireLoadModel(const std::string &wlm_name) {
  if ((wlm_name == "sky") || (wlm_name == "5k") ||
      (wlm_name == "3k")  || (wlm_name == "1k") ) {

    wire_load_name = wlm_name;
    float length;
    float mp_sky[] = {1.0, 1.38, 2.08, 2.75, 3.71, 3.62};
    float mp_5k[] = {1.0, 2.1, 3.53, 5.51, 8.31, 11.70};
    float mp_3k[] = {1.0, 2.49, 3.20, 6.19, 8.59, 11.50};
    float mp_1k[] = {1.0, 2.26, 3.70, 5.28, 6.82, 8.50};
    float multip[6];

    for (size_t i = 0; i < 6; ++i)
      fanout_length[i].first = i+1;

    if (wire_load_name == "sky") {
      length = length_sky;
      for (size_t i = 0; i < 6; ++i)
        multip[i] = mp_sky[i];

      /// Changing properties
      r = fudge * 0.08;
      c = fudge * 0.0002;
      slope = 8.3631;
    }
    else if (wire_load_name == "5k") {
      length = length_5k;
      for (size_t i = 0; i < 6; ++i)
        multip[i] = mp_5k[i];

      /// Changing properties
      r = fudge * 0.004;
      c = fudge * 0.2;
      slope = 6.2836;
    }
    else if (wire_load_name == "3k") {
      length = length_3k;
      for (size_t i = 0; i < 6; ++i)
        multip[i] = mp_3k[i];

      /// Changing properties
      r = fudge * 0.004;
      c = fudge * 0.2;
      slope = 6.2836;
    }
    else if (wire_load_name == "1k") {
      length = length_1k;
      for (size_t i = 0; i < 6; ++i)
        multip[i] = mp_1k[i];

      /// Changing properties
      r = fudge * 0.004;
      c = fudge * 0.2;
      slope = 6.2836;
    }

    for (size_t i = 0; i < 6; ++i)
      fanout_length[i].second = length*multip[i];

    for (size_t i = 0; i < 6; ++i)
      fanout_resistance[i] = std::make_pair(i+1, fanout_length[i].second * r);

    for (size_t i = 0; i < 6; ++i)
      fanout_capacitance[i] = std::make_pair(i+1, fanout_length[i].second * c);
  }

  else {
    std::cerr << "WLM: wrong name\n";
    assert(false);
  }
}

/// Getters
float WLM::getLength(const std::size_t &fanoutCount) const {
  if ((fanoutCount > 0) && (fanoutCount < 6))
    return fanout_length[fanoutCount-1].second;
  else if (fanoutCount > 6)
    return fanout_length[5].second + (fanoutCount - 6) * slope;
  else {
    std::cerr << "WLM: wrong name\n";
    assert(false);
  }

  return 0;
}

float WLM::getFanoutCap(const std::size_t &fanoutCount) const {
  return getLength(fanoutCount) * c;
}

float WLM::getFanoutRes(const std::size_t &fanoutCount) const {
  if ((fanoutCount > 0) && (fanoutCount < 6))
    return fanout_resistance[fanoutCount-1].second;
  else if (fanoutCount > 6) {
    float length = fanout_length[5].second + (fanoutCount - 6) * slope;
    return length * r;
  }
  else {
    std::cerr << "WLM: wrong name\n";
    assert(false);
  }

  return 0;
}

/// Interpolation
float NLDM::getLutValue(const std::vector<float> &lutValues,
                        DataTimingContext &context,
                        const float x1, const float x2,
                        const float y1, const float y2) {
  if (!context.interpolate) {
    return lutValues[context.index.ind1 * context.variablesCount + context.index.ind2];
  }

  /// Properties
  float x0 = context.inputTransTime;
  float y0 = context.outputTotalCap;
  float x01 = 0, x20 = 0, y01 = 0, y20 = 0;
  float T00 = 0, T11 = 0, T12 = 0, T21 = 0, T22 = 0;

  /// Estimation
  x01 = (x0 - x1) / (x2 - x1);
  x20 = (x2 - x0) / (x2 - x1);
  y01 = (y0 - y1) / (y2 - y1);
  y20 = (y2 - y0) / (y2 - y1);

  T11 = lutValues[context.index.back1 * context.variablesCount + context.index.back2];
  T12 = lutValues[context.index.back1 * context.variablesCount + context.index.front2];
  T21 = lutValues[context.index.front1 * context.variablesCount + context.index.back2];
  T22 = lutValues[context.index.front1 * context.variablesCount + context.index.front2];

  /// Result value
  T00 = x20*y20*T11 + x20*y01*T12 + x01*y20*T21 + x01*y01*T22;
  return T00;
}

float NLDM::getLutValue(const LookupTable *lut,
                        DataTimingContext &context,
                        const float x1, const float x2,
                        const float y1, const float y2) {
  std::vector<float> lutValues = lut->getValues();

  return getLutValue(lutValues, context, x1, x2, y1, y2);
}

void NLDM::pinTimingEstimator(const std::vector<const LookupTable*> &luts,
                              DataTimingContext &context) {

  //===-----------------------------------------------------------------===//
  //  Properties
  //===-----------------------------------------------------------------===//
  std::vector<float> lutValues;
  /// Values for interpolation
  float x1 = 0, x2 = 0, y1 = 0, y2 = 0;

  //===-----------------------------------------------------------------===//
  //  Assign values from LookUp Tables
  //===-----------------------------------------------------------------===//
  if (luts[0] != nullptr) {
    //===-----------------------------------------------------------------===//
    //  Properties
    //===-----------------------------------------------------------------===//
    lutValues = luts[0]->getValues();
    bool iterFlag = false;
    //===-----------------------------------------------------------------===//
    for (const auto &it : (*luts[0])) {
      /// INTERPOLATION
      if (std::find(it.values.begin(), it.values.end(),
            context.inputTransTime) == std::end(it.values)) {
        if (!iterFlag) {
          /// Assign net_transition value
          for (size_t i = 0; i < it.values.size(); ++i) {
            if (it.values[i] < context.inputTransTime) {
              context.index.back1 = i;
              x1 = it.values[i];
            }
            else if (it.values[i] > context.inputTransTime) {
              context.index.front1 = i;
              x2 = it.values[i];
              context.variablesCount = it.values.size();
              iterFlag = true;
              break;
            }
          }
        }
        else {
          /// Assign output capacitance value
          for (size_t i = 0; i < it.values.size(); ++i) {
            if (it.values[i] < context.outputTotalCap) {
              context.index.back2 = i;
              y1 = it.values[i];
            }
            else if (it.values[i] > context.outputTotalCap) {
              context.index.front2 = i;
              y2 = it.values[i];
              break;
            }
          }
          bool tmpInterpolate = context.interpolate; // TODO
          context.interpolate = true;
          context.delayValues.push_back(
                getLutValue(lutValues, context, x1, x2, y1, y2));
          context.interpolate = tmpInterpolate;
          break;
        }
      }

      /// Assign found values
      else if (std::find(it.values.begin(), it.values.end(),
                            context.inputTransTime) != std::end(it.values)) {
        if (!iterFlag) {
          /// Assign net_transition value
          for (size_t i = 0; i < it.values.size(); ++i) {
            if (it.values[i] == context.inputTransTime) {
              context.index.ind1 = i;
              iterFlag = true;
              break;
            }
          }
        }
        else {
          /// Assign output capacitance value
          for (size_t i = 0; i < it.values.size(); ++i) {
            if (it.values[i] == context.outputTotalCap) {
              context.index.ind2 = i;
              context.variablesCount = it.values.size();
              break;
            }
          }
          context.delayValues.push_back(
            lutValues[context.index.ind1 * context.variablesCount + context.index.ind2]);
          break;
        }
      }
    }
  }
  else {
    std::cerr << "Cell fall LUT is nullptr.\n";
    assert(false);
  }

  context.interpolate = (context.index.ind1 == -1) && (context.index.ind2 == -1);

  for (size_t i = 1; i < 4; ++i) {
    if (luts[i] != nullptr)
      if (luts[i]->getValues().size() != 1)
        context.delayValues.push_back(
          getLutValue(luts[i], context, x1, x2, y1, y2));
      else
        context.delayValues.push_back(0);
    else {
      std::cerr << "LUT is nullptr.\n";
      assert(false);
    }
  }
}

void NLDM::pinFTimingEstimator(const std::vector<const LookupTable*> &luts,
                               DataTimingContext &context) {
  //===-----------------------------------------------------------------===//
  //  Properties
  //===-----------------------------------------------------------------===//
  std::vector<float> lutValues, result;
  //===-----------------------------------------------------------------===//
  for (size_t i = 0; i < 4; ++i) {
    if (luts[i] != nullptr) {
      lutValues = luts[i]->getValues();
      result.push_back(
        lutValues[context.index.ind1 * context.variablesCount + context.index.ind2]);
    }
    else {
      /// Found cell has a "scalar" LUT
      result.push_back(0);
    }
  }

  context.delayValues = result;
}

void NLDM::pinITimingEstimator(const std::vector<const LookupTable*> &luts,
                               DataTimingContext &context) {
  //===-----------------------------------------------------------------===//
  //  Properties
  //===-----------------------------------------------------------------===//
  std::vector<float> lutValues, result;
  float x1 = 0, x2 = 0,
        y1 = 0, y2 = 0;
  bool iterFlag = false;

  assert(luts[0] != nullptr);
  for (const auto &it : (*luts[0])) {
    if (!iterFlag) {
      x1 = it.values[context.index.back1], x2 = it.values[context.index.front1];
      iterFlag = true;
    }
    else {
      y1 = it.values[context.index.back2], y2 = it.values[context.index.front2];
    }
  }
  //===-----------------------------------------------------------------===//
  bool tmpInterpolation = context.interpolate; // TODO
  context.interpolate = true;
  for (size_t i = 0; i < 4; ++i) {
    if (luts[i] != nullptr) {
      lutValues = luts[i]->getValues();
      result.push_back(getLutValue(lutValues, context, x1, x2, y1, y2));
    }
    else {
      /// Found cell has a "scalar" LUT
      result.push_back(0);
    }
  }
  context.interpolate = tmpInterpolation;
  context.delayValues = result;
}

void NLDM::delayEstimation(Library &library, const std::string &cellType,
                           const float inputTransTime,
                           const float outputTotalCap,
                           int &timingSense, float &slew, float &delay, float &cap) {
  //===---------------------------------------------------------------===//
  //  Properties
  //===---------------------------------------------------------------===//
  std::vector<float> cfall, crise, tfall, trise;
  bool readyFlag = false;
  int newTimingSense = 0;
  DataTimingContext context{inputTransTime, outputTotalCap};

  //===---------------------------------------------------------------===//
  //  Delay and Slew estimation
  //===---------------------------------------------------------------===//
  const Cell *cell = library.getCell(cellType);

  cap = 0;
  for (const Pin &pin : (*cell).getPins()) {
    cap += pin.getFloatAttribute("capacitance", 0);
    for (const Timing &timing : pin.getTimings()) {
      //===---------------------------------------------------------------===//
      //  LookUp Tables
      //===---------------------------------------------------------------===//
      std::vector<const LookupTable*> luts {
        timing.getLut("cell_fall"),
        timing.getLut("cell_rise"),
        timing.getLut("fall_transition"),
        timing.getLut("rise_transition")};

      if (!readyFlag) {
        context.interpolate = true;
        pinTimingEstimator(luts, context);
        readyFlag = true;
      }
      else {
        if (!context.interpolate) {
          pinFTimingEstimator(luts, context);
        }
        else {
          pinITimingEstimator(luts, context);
        }
      }
      cfall.push_back(context.delayValues[0]);
      crise.push_back(context.delayValues[1]);
      tfall.push_back(context.delayValues[2]);
      trise.push_back(context.delayValues[3]);
      newTimingSense = timing.getIntegerAttribute("timing_sense", 0);
    }
  }

  if (timingSense == 0) {
    delay = *max_element(crise.begin(), crise.end());
    for (size_t i = 0; i < crise.size(); ++i)
      if (crise[i] == delay) {
        slew = (tfall[i] + trise[i])/2;
        break;
      }
  }
  else {
    delay = *max_element(cfall.begin(), cfall.end());
    for (size_t i = 0; i < cfall.size(); ++i)
      if (cfall[i] == delay) {
        slew = (tfall[i] + trise[i])/2;
        break;
      }
  }
  timingSense = newTimingSense;
}

float NLDM::delayEstimation(Library &library, const std::string &cellTypeName,
    const float inputTransTime, const float outputTotalCap) {
  int timingSense = 0;
  float slew = 0;
  float delay = 0;
  float cap = 0;
  delayEstimation(library, cellTypeName, inputTransTime, outputTotalCap,
    timingSense, slew, delay, cap);
  return slew;
}
} // namespace eda::gate::estimator
